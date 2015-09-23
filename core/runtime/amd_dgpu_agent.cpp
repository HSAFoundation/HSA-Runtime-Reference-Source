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
#include "core/inc/amd_dgpu_agent.h"

#include <algorithm>
#include <vector>
#include <cstring>
#include <climits>

#include "core/inc/amd_blit_kernel.h"
#include "core/inc/amd_cpu_agent.h"
#include "core/inc/amd_memory_region.h"
#include "core/inc/amd_hw_aql_command_processor.h"
#include "core/inc/runtime.h"
#include "core/runtime/isa.hpp"

#include "inc/hsa_ext_image.h"

namespace amd {

DGpuAgent::DGpuAgent(HSAuint32 node, const HsaNodeProperties& node_props,
                     const std::vector<HsaCacheProperties>& cache_props)
    : GpuAgent(node, node_props, cache_props) {
  // TODO(bwicakso): remove dummy code when KFD provide correct topology.

  // Dummy CPU agent.
  HsaNodeProperties node_prop = {0};
  std::vector<HsaCacheProperties> cpu_cache_props(0);
  CpuAgent* cpu = new CpuAgent(0, node_prop, cpu_cache_props);
  core::Runtime::runtime_singleton_->RegisterAgent(cpu);

  // Dummy node properties.
  HsaNodeProperties& mod_prop = (HsaNodeProperties&)properties_;
  mod_prop.NumFComputeCores = 112;
  mod_prop.NumSIMDPerCU = 4;
  mod_prop.MaxWavesPerSIMD = 40;
  mod_prop.LDSSizeInKB = 64;
  mod_prop.WaveFrontSize = 64;
  mod_prop.NumShaderBanks = 2;
  mod_prop.NumCUPerArray = 16;
  mod_prop.MaxSlotsScratchCU = 128;
  mod_prop.MaxEngineClockMhzFCompute = 720;

  scratch_pool_. ~SmallHeap();
  new (&scratch_pool_) SmallHeap((void*)0x1000, 4294963200);

  // Dummy system region.
  HsaMemoryProperties system_props;
  std::memset(&system_props, 0, sizeof(HsaMemoryProperties));

  system_props.HeapType = HSA_HEAPTYPE_SYSTEM;
  system_props.SizeInBytes = (HSAuint64)(0x200000000);
  system_props.VirtualBaseAddress = (HSAuint64)(0);

  MemoryRegion* system_region = new MemoryRegion(false, 0, system_props);
  core::Runtime::runtime_singleton_->RegisterMemoryRegion(system_region);

  // Dummy LDS region.
  HsaMemoryProperties lds_props;
  std::memset(&lds_props, 0, sizeof(HsaMemoryProperties));

  lds_props.HeapType = HSA_HEAPTYPE_GPU_LDS;
  lds_props.SizeInBytes = (HSAuint64)(64 * 1024);
  lds_props.VirtualBaseAddress = (HSAuint64)(0x2000000000000000);

  MemoryRegion* lds_region = new MemoryRegion(false, 0, lds_props);
  core::Runtime::runtime_singleton_->RegisterMemoryRegion(lds_region);
  RegisterMemoryProperties(*lds_region);

  // Dummy scratch region.
  HsaMemoryProperties scratch_props;
  std::memset(&scratch_props, 0, sizeof(HsaMemoryProperties));

  scratch_props.HeapType = HSA_HEAPTYPE_GPU_SCRATCH;
#if defined(HSA_LARGE_MODEL) && defined(__linux__)
  scratch_props.SizeInBytes = static_cast<HSAuint64>(4294967296);
#else
  scratch_props.SizeInBytes = static_cast<HSAuint64>(335544320);
#endif
  scratch_props.VirtualBaseAddress = (HSAuint64)(0);

  MemoryRegion* scratch_region = new MemoryRegion(false, 0, scratch_props);
  core::Runtime::runtime_singleton_->RegisterMemoryRegion(scratch_region);
  RegisterMemoryProperties(*scratch_region);

  // Dummy scratch pool.
  scratch_per_thread_ = 2048;
  queue_scratch_len_ = AlignUp(32 * 64 * 8 * scratch_per_thread_, 65536);

  // Set compute_capability_ via node property, only on GPU device.
  compute_capability_.Initialize(8, 0, 1);
}

DGpuAgent::~DGpuAgent() {
  scratch_pool_. ~SmallHeap();
  new (&scratch_pool_) SmallHeap();
}

void DGpuAgent::RegisterMemoryProperties(core::MemoryRegion& region) {
  MemoryRegion* amd_region = reinterpret_cast<MemoryRegion*>(&region);

  assert((!amd_region->IsGDS()) &&
         ("Memory region should only be global, group or scratch"));

  regions_.push_back(amd_region);
}

hsa_status_t DGpuAgent::GetInfo(hsa_agent_info_t attribute, void* value) const {
  const size_t kNameSize = 64;  // agent, and vendor name size limit

  const core::ExtensionEntryPoints& extensions =
      core::Runtime::runtime_singleton_->extensions_;

  hsa_agent_t agent = core::Agent::Convert(this);

  const size_t attribute_u = static_cast<size_t>(attribute);
  switch (attribute_u) {
    case HSA_AGENT_INFO_NAME:
      // TODO(bwicakso): hardcode for now.
      std::memset(value, 0, kNameSize);
      std::memcpy(value, "Tonga", sizeof("Tonga"));
      break;
    case HSA_AGENT_INFO_PROFILE:
      *((hsa_profile_t*)value) = HSA_PROFILE_BASE;
      break;
    case HSA_AGENT_INFO_QUEUES_MAX:
      *((uint32_t*)value) = 8;
      break;
    case HSA_AGENT_INFO_EXTENSIONS:
      memset(value, 0, sizeof(uint8_t) * 128);

      if (extensions.table.hsa_ext_program_finalize_fn != NULL) {
        *((uint8_t*)value) = 1 << HSA_EXTENSION_FINALIZER;
      }

      // TODO(bwicakso): temporarily disable image extension support on dgpu
      break;

    default:
      return GpuAgent::GetInfo(attribute, value);
      break;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t DGpuAgent::QueueCreate(size_t size, hsa_queue_type_t queue_type,
                                    core::HsaEventCallback event_callback,
                                    void* data, uint32_t private_segment_size,
                                    uint32_t group_segment_size,
                                    core::Queue** queue) {
  // TODO(bwicakso): not implemented yet.
  return HSA_STATUS_ERROR;
}

}  // namespace amd