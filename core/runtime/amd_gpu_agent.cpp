////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation(if any)
// (collectively, the "Materials") pursuant to the terms and conditions of the
// Software License Agreement included with the Materials.If you do not have a
// copy of the Software License Agreement, contact your AMD representative for a
// copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER : THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE.THE ENTIRE RISK ASSOCIATED WITH THE USE OF
// THE SOFTWARE IS ASSUMED BY YOU.Some jurisdictions do not allow the exclusion
// of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION : AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD.  You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS : The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor.Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////
#include "core/inc/amd_gpu_agent.h"

#include <algorithm>
#include <vector>
#include <cstring>
#include <climits>

#include "core/inc/amd_blit_kernel.h"
#include "core/inc/runtime.h"
#include "core/inc/amd_memory_region.h"
#include "core/inc/amd_hw_aql_command_processor.h"
#include "core/runtime/isa.hpp"

#include "inc/hsa_ext_image.h"

// Size of scratch (private) segment pre-allocated per thread, in bytes.
#define DEFAULT_SCRATCH_BYTES_PER_THREAD 2048

namespace amd {
void ReleaseApe1(void* addr, size_t size) {
  assert(addr != NULL && size != 0);
  _aligned_free(addr);
}

HsaMemoryProperties ReserveApe1(HSAuint32 node_id, size_t size,
                                size_t alignment) {
  // Only valid in 64 bit.
  assert(sizeof(void*) == 8);

  HsaMemoryProperties ape1_prop;

  void* ape1 = _aligned_malloc(size, alignment);
  assert((ape1 != NULL) && ("APE1 allocation failed"));

  if (HSAKMT_STATUS_SUCCESS !=
      hsaKmtSetMemoryPolicy(node_id, HSA_CACHING_CACHED, HSA_CACHING_NONCACHED,
                            ape1, size)) {
    ReleaseApe1(ape1, size);
    std::memset(&ape1_prop, 0, sizeof(ape1_prop));
    assert(false && "hsaKmtSetMemoryPolicy failed");
    return ape1_prop;
  }

  std::memset(&ape1_prop, 0, sizeof(ape1_prop));
  ape1_prop.HeapType = HSA_HEAPTYPE_SYSTEM;
  ape1_prop.SizeInBytes = size;
  ape1_prop.VirtualBaseAddress = reinterpret_cast<HSAuint64>(ape1);

  return ape1_prop;
}

GpuAgent::GpuAgent(HSAuint32 node, const HsaNodeProperties& node_props,
                   const std::vector<HsaCacheProperties>& cache_props)
    : node_id_(node),
      properties_(node_props),
      cache_props_(cache_props),
      ape1_base_(0),
      ape1_size_(0),
      current_coherency_type_(HSA_AMD_COHERENCY_TYPE_COHERENT),
      blit_(NULL) {
  HSAKMT_STATUS err = hsaKmtGetClockCounters(node_id_, &t0_);
  t1_ = t0_;
  assert(err == HSAKMT_STATUS_SUCCESS && "hsaGetClockCounters error");

  HsaMemFlags flags;
  flags.Value = 0;
  flags.ui32.Scratch = 1;
  flags.ui32.HostAccess = 1;

  scratch_per_thread_ = atoi(os::GetEnvVar("HSA_SCRATCH_MEM").c_str());
  if (scratch_per_thread_ == 0)
    scratch_per_thread_ = DEFAULT_SCRATCH_BYTES_PER_THREAD;

  int queues = atoi(os::GetEnvVar("HSA_MAX_QUEUES").c_str());
#if !defined(HSA_LARGE_MODEL) || !defined(__linux__)
  if (queues == 0) queues = 10;
#endif

  // Scratch length is: waves/CU * threads/wave * queues * #CUs *
  // scratch/thread
  queue_scratch_len_ = 0;
  queue_scratch_len_ = AlignUp(32 * 64 * 8 * scratch_per_thread_, 65536);
  size_t scratchLen = queue_scratch_len_ * queues;

// For 64-bit linux use max queues unless otherwise specified
#if defined(HSA_LARGE_MODEL) && defined(__linux__)
  if ((scratchLen == 0) || (scratchLen > 4294967296))
    scratchLen = 4294967296;  // 4GB apeture max
#endif

  void* scratchBase;
  err = hsaKmtAllocMemory(node_id_, scratchLen, flags, &scratchBase);
  assert(err == HSAKMT_STATUS_SUCCESS && "hsaKmtAllocMemory(Scratch) failed");
  assert(IsMultipleOf(scratchBase, 0x1000) &&
         "Scratch base is not page aligned!");

  scratch_pool_. ~SmallHeap();
  new (&scratch_pool_) SmallHeap(scratchBase, scratchLen);

  if (sizeof(void*) == 8) {
    // 64 bit only. Setup APE1 memory region, which contains
    // non coherent memory.

    static const size_t kApe1Alignment = 64 * 1024;
    static const size_t kApe1Size = kApe1Alignment;

    const HsaMemoryProperties ape1_prop =
        ReserveApe1(node_id_, kApe1Size, kApe1Alignment);

    if (ape1_prop.SizeInBytes > 0) {
      SetApe1BaseAndSize((uintptr_t)ape1_prop.VirtualBaseAddress,
                         (size_t)ape1_prop.SizeInBytes);
    }
  }
}

GpuAgent::~GpuAgent() {
  if (ape1_base_ != 0) {
    ReleaseApe1(reinterpret_cast<void*>(ape1_base_), ape1_size_);
  }

  regions_.clear();

  if (scratch_pool_.base() != NULL) {
    hsaKmtFreeMemory(scratch_pool_.base(), scratch_pool_.size());
  }

  if (blit_ != NULL) {
    hsa_status_t status = blit_->Destroy();
    assert(status == HSA_STATUS_SUCCESS);

    delete blit_;
  }
}

void GpuAgent::RegisterMemoryProperties(core::MemoryRegion& region) {
  MemoryRegion* amd_region = reinterpret_cast<MemoryRegion*>(&region);

  assert((!amd_region->IsGDS()) &&
         ("Memory region should only be global, group or scratch"));

  regions_.push_back(amd_region);
}

hsa_status_t GpuAgent::IterateRegion(
    hsa_status_t (*callback)(hsa_region_t region, void* data),
    void* data) const {
  const size_t num_mems = regions().size();

  for (size_t j = 0; j < num_mems; ++j) {
    const MemoryRegion* amd_region =
        reinterpret_cast<const MemoryRegion*>(regions()[j]);

    // Skip memory regions other than host, gpu local, or LDS.
    if (amd_region->IsSystem() || amd_region->IsLocalMemory() ||
        amd_region->IsLDS()) {
      hsa_region_t hsa_region = core::MemoryRegion::Convert(amd_region);
      hsa_status_t status = callback(hsa_region, data);
      if (status != HSA_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t GpuAgent::DmaCopy(void* dst, const void* src, size_t size) {
  if (blit_ == NULL) {
    ScopedAcquire<KernelMutex> Lock(&lock_);
    if (blit_ == NULL) {
      // TODO: temporary fallback to kernel approach until SDMA is functional.
      blit_ = new BlitKernel();
      assert(blit_ != NULL);
      hsa_status_t status =
          blit_->Initialize(*core::Agent::Convert(public_handle()));
    }
  }

  return blit_->SubmitLinearCopyCommand(dst, src, size);
}

hsa_status_t GpuAgent::GetInfo(hsa_agent_info_t attribute, void* value) const {
  const size_t kNameSize = 64;  // agent, and vendor name size limit

  const core::ExtensionEntryPoints& extensions =
      core::Runtime::runtime_singleton_->extensions_;

  hsa_agent_t agent = core::Agent::Convert(this);

  const size_t attribute_u = static_cast<size_t>(attribute);
  switch (attribute_u) {
    case HSA_AGENT_INFO_NAME:
      // TODO: hardcode for now.
      std::memset(value, 0, kNameSize);
      std::memcpy(value, "Spectre", sizeof("Spectre"));
      break;
    case HSA_AGENT_INFO_VENDOR_NAME:
      std::memset(value, 0, kNameSize);
      std::memcpy(value, "AMD", sizeof("AMD"));
      break;
    case HSA_AGENT_INFO_FEATURE:
      *((hsa_agent_feature_t*)value) = HSA_AGENT_FEATURE_KERNEL_DISPATCH;
      break;
    case HSA_AGENT_INFO_MACHINE_MODEL:
#if defined(HSA_LARGE_MODEL)
      *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_LARGE;
#else
      *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_SMALL;
#endif
      break;
    case HSA_AGENT_INFO_BASE_PROFILE_DEFAULT_FLOAT_ROUNDING_MODES:
    case HSA_AGENT_INFO_DEFAULT_FLOAT_ROUNDING_MODE:
      *((hsa_default_float_rounding_mode_t*)value) =
          HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR;
      break;
    case HSA_AGENT_INFO_FAST_F16_OPERATION:
      *((bool*)value) = false;
      break;
    case HSA_AGENT_INFO_PROFILE:
      *((hsa_profile_t*)value) = HSA_PROFILE_FULL;
      break;
    case HSA_AGENT_INFO_WAVEFRONT_SIZE:
      *((uint32_t*)value) = properties_.WaveFrontSize;
      break;
    case HSA_AGENT_INFO_WORKGROUP_MAX_DIM: {
      // TODO: must be per-device
      const uint16_t group_size[3] = {2048, 2048, 2048};
      std::memcpy(value, group_size, sizeof(group_size));
    } break;
    case HSA_AGENT_INFO_WORKGROUP_MAX_SIZE:
      // TODO: must be per-device
      *((uint32_t*)value) = 2048;
      break;
    case HSA_AGENT_INFO_GRID_MAX_DIM: {
      const hsa_dim3_t grid_size = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
      std::memcpy(value, &grid_size, sizeof(hsa_dim3_t));
    } break;
    case HSA_AGENT_INFO_GRID_MAX_SIZE:
      *((uint32_t*)value) = UINT32_MAX;
      break;
    case HSA_AGENT_INFO_FBARRIER_MAX_SIZE:
      // TODO: to confirm
      *((uint32_t*)value) = 32;
      break;
    case HSA_AGENT_INFO_QUEUES_MAX:
      *((uint32_t*)value) = 128;
      break;
    case HSA_AGENT_INFO_QUEUE_MIN_SIZE:
      *((uint32_t*)value) = minAqlSize_;
      break;
    case HSA_AGENT_INFO_QUEUE_MAX_SIZE:
      *((uint32_t*)value) = maxAqlSize_;
      break;
    case HSA_AGENT_INFO_QUEUE_TYPE:
      *((hsa_queue_type_t*)value) = HSA_QUEUE_TYPE_MULTI;
      break;
    case HSA_AGENT_INFO_NODE:
      // TODO: associate with OS NUMA support (numactl / GetNumaProcessorNode).
      *((uint32_t*)value) = node_id_;
      break;
    case HSA_AGENT_INFO_DEVICE:
      *((hsa_device_type_t*)value) = HSA_DEVICE_TYPE_GPU;
      break;
    case HSA_AGENT_INFO_CACHE_SIZE:
      std::memset(value, 0, sizeof(uint32_t) * 4);
      // TODO: no GPU cache info from KFD. Hardcode for now.
      // GCN whitepaper: L1 data cache is 16KB.
      ((uint32_t*)value)[0] = 16 * 1024;
      break;
    case HSA_AGENT_INFO_ISA:
      return core::Isa::Create(agent, (hsa_isa_t*)value);
    case HSA_AGENT_INFO_EXTENSIONS:
      memset(value, 0, sizeof(uint8_t) * 128);

      if (extensions.table.hsa_ext_program_finalize_fn != NULL) {
        *((uint8_t*)value) = 1 << HSA_EXTENSION_FINALIZER;
      }

      if (extensions.table.hsa_ext_image_create_fn != NULL) {
        *((uint8_t*)value) |= 1 << HSA_EXTENSION_IMAGES;
      }

      *((uint8_t*)value) |= 1 << HSA_EXTENSION_AMD_PROFILER;

      break;
    case HSA_AGENT_INFO_VERSION_MAJOR:
      *((uint16_t*)value) = 1;
      break;
    case HSA_AGENT_INFO_VERSION_MINOR:
      *((uint16_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_1D_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_1DA_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_1DB_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2D_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DA_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DDEPTH_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DADEPTH_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_3D_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_ARRAY_MAX_LAYERS:
      return hsa_amd_image_get_info_max_dim(public_handle(), attribute, value);
    case HSA_EXT_AGENT_INFO_MAX_IMAGE_RD_HANDLES:
      // TODO: hardcode based on OCL constants.
      *((uint32_t*)value) = 128;
      break;
    case HSA_EXT_AGENT_INFO_MAX_IMAGE_RORW_HANDLES:
      // TODO: hardcode based on OCL constants.
      *((uint32_t*)value) = 64;
      break;
    case HSA_EXT_AGENT_INFO_MAX_SAMPLER_HANDLERS:
      // TODO: hardcode based on OCL constants.
      *((uint32_t*)value) = 16;
    case HSA_AMD_AGENT_INFO_CHIP_ID:
      *((uint32_t*)value) = properties_.DeviceId;
      break;
    case HSA_AMD_AGENT_INFO_CACHELINE_SIZE:
      // TODO: hardcode for now.
      // GCN whitepaper: cache line size is 64 byte long.
      *((uint32_t*)value) = 64;
      break;
    case HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT:
      *((uint32_t*)value) =
          (properties_.NumFComputeCores / properties_.NumSIMDPerCU);
      break;
    case HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY:
      *((uint32_t*)value) = properties_.MaxEngineClockMhzFCompute;
      break;
    case HSA_AMD_AGENT_INFO_DRIVER_NODE_ID:
      *((uint32_t*)value) = node_id_;
      break;
    case HSA_AMD_AGENT_INFO_MAX_ADDRESS_WATCH_POINTS:
      *((uint32_t*)value) = static_cast<uint32_t>(
          1 << properties_.Capability.ui32.WatchPointsTotalBits);
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
      break;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t GpuAgent::QueueCreate(size_t size, hsa_queue_type_t queue_type,
                                   core::HsaEventCallback event_callback,
                                   void* data, uint32_t private_segment_size,
                                   uint32_t group_segment_size,
                                   core::Queue** queue) {
  // AQL queues must be a power of two in length.
  if (!IsPowerOfTwo(size)) return HSA_STATUS_ERROR_INVALID_ARGUMENT;

  // Enforce max size
  if (size > maxAqlSize_) return HSA_STATUS_ERROR_OUT_OF_RESOURCES;

  // Allocate scratch memory
  ScratchInfo scratch;
#if defined(HSA_LARGE_MODEL) && defined(__linux__)
  if (core::g_use_interrupt_wait) {
    if (private_segment_size == UINT_MAX)
      private_segment_size = scratch_per_thread_;
    if (private_segment_size > 262128) return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    scratch.size_per_thread = AlignUp(private_segment_size, 16);
    if (scratch.size_per_thread > 262128)
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    uint32_t CUs = properties_.NumFComputeCores / properties_.NumSIMDPerCU;
    // TODO: Replace constants with proper topology data.
    scratch.size = scratch.size_per_thread * 32 * 64 * CUs;
  } else {
    scratch.size = queue_scratch_len_;
    scratch.size_per_thread = scratch_per_thread_;
  }
#else
  scratch.size = queue_scratch_len_;
  scratch.size_per_thread = scratch_per_thread_;
#endif
  scratch.queue_base = NULL;
  if (scratch.size != 0) {
    AcquireQueueScratch(scratch);
    if (scratch.queue_base == NULL) return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  // Create an HW AQL queue
  HwAqlCommandProcessor* hw_queue = new HwAqlCommandProcessor(
      this, size, node_id_, scratch, event_callback, data);
  if (hw_queue && hw_queue->IsValid()) {
    // return queue
    *queue = hw_queue;
    return HSA_STATUS_SUCCESS;
  }
  // If reached here its always an ERROR.
  delete hw_queue;
  ReleaseQueueScratch(scratch.queue_base);
  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

void GpuAgent::TranslateTime(core::Signal* signal,
                             hsa_amd_profiling_dispatch_time_t& time) {
  // Ensure interpolation
  if (t1_.GPUClockCounter < signal->signal_.end_ts) SyncClocks();

  time.start = uint64_t(
      (double(int64_t(t0_.SystemClockCounter - t1_.SystemClockCounter)) /
       double(int64_t(t0_.GPUClockCounter - t1_.GPUClockCounter))) *
          double(int64_t(signal->signal_.start_ts - t1_.GPUClockCounter)) +
      double(t1_.SystemClockCounter));
  time.end = uint64_t(
      (double(int64_t(t0_.SystemClockCounter - t1_.SystemClockCounter)) /
       double(int64_t(t0_.GPUClockCounter - t1_.GPUClockCounter))) *
          double(int64_t(signal->signal_.end_ts - t1_.GPUClockCounter)) +
      double(t1_.SystemClockCounter));
}

bool GpuAgent::current_coherency_type(hsa_amd_coherency_type_t type) {
  ScopedAcquire<KernelMutex> Lock(&lock_);
  if (type == current_coherency_type_) {
    return true;
  }

  HSA_CACHING_TYPE type0, type1;
  if (current_coherency_type_ == HSA_AMD_COHERENCY_TYPE_COHERENT) {
    type0 = HSA_CACHING_NONCACHED;
    type1 = HSA_CACHING_CACHED;
    type = HSA_AMD_COHERENCY_TYPE_NONCOHERENT;
  } else {
    type0 = HSA_CACHING_CACHED;
    type1 = HSA_CACHING_NONCACHED;
    type = HSA_AMD_COHERENCY_TYPE_COHERENT;
  }

  if (hsaKmtSetMemoryPolicy(node_id_, type0, type1,
                            reinterpret_cast<void*>(ape1_base_),
                            ape1_size_) != HSAKMT_STATUS_SUCCESS) {
    return false;
  }
  current_coherency_type_ = type;
  return true;
}

uint16_t GpuAgent::GetMicrocodeVersion() const {
  return uint16_t(properties_.EngineId & 0xFFFF);
}

void GpuAgent::SyncClocks() {
  HSAKMT_STATUS err = hsaKmtGetClockCounters(node_id_, &t1_);
  assert(err == HSAKMT_STATUS_SUCCESS && "hsaGetClockCounters error");
}

}  // namespace
