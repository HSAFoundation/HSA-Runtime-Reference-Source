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

#include "core/inc/amd_memory_region.h"

#include "core/inc/runtime.h"
#include "core/inc/amd_cpu_agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/thunk.h"

namespace amd {
/// @brief Allocate component accessible memory (system / local memory).
static void* AllocateKfdMemory(const HsaMemFlags& flag, HSAuint32 node_id,
                               size_t size) {
  void* ret = NULL;
  const HSAKMT_STATUS status = hsaKmtAllocMemory(node_id, size, flag, &ret);

  HSAuint64 alternate_va;
  hsaKmtMapMemoryToGPU(ret, size, &alternate_va);

  return (status == HSAKMT_STATUS_SUCCESS) ? ret : NULL;
}

/// @brief Free component accessible memory (system / local memory).
static void FreeKfdMemory(void* ptr, size_t size) {
  if (ptr == NULL || size == 0) {
    return;
  }

  hsaKmtUnmapMemoryToGPU(ptr);
  HSAKMT_STATUS status = hsaKmtFreeMemory(ptr, size);
  assert(status == HSAKMT_STATUS_SUCCESS);
}

MemoryRegion::MemoryRegion(const core::Agent& agent,
                           const HsaMemoryProperties& mem_props)
    : core::MemoryRegion(agent), mem_props_(mem_props) {
  mem_flag_.Value = 0;
  if (IsLocalMemory()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = 0;
    mem_flag_.ui32.NonPaged = 1;
  } else if (IsSystem()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = 1;
    mem_flag_.ui32.CachePolicy = HSA_CACHING_CACHED;
  }
}

MemoryRegion::~MemoryRegion() {}

hsa_status_t MemoryRegion::Allocate(size_t size, void** address) const {
  if (address == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  HSAuint32 node_id = 0;
  if (agent()->device_type() == core::Agent::kAmdGpuDevice) {
    node_id = static_cast<const amd::GpuAgent*>(agent())->node_id();
  } else {
    node_id = static_cast<const amd::CpuAgent*>(agent())->node_id();
  }

  *address = amd::AllocateKfdMemory(mem_flag_, node_id, size);

  return (*address != NULL) ? HSA_STATUS_SUCCESS
                            : HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

hsa_status_t MemoryRegion::Free(void* address, size_t size) const {
  amd::FreeKfdMemory(address, size);

  return HSA_STATUS_SUCCESS;
}

static __forceinline uint32_t
    CalculateDDR3Bandwidth(uint32_t clock, uint32_t width) {
  // clock * 4 (bus clock multiplier) * 2 (double data rate) * width (in bits) /
  // 8 (bits in byte)
  return clock * width;
}

hsa_status_t MemoryRegion::GetInfo(hsa_region_info_t attribute,
                                   void* value) const {
  uint32_t bandwidth_multiplier = 1;
  switch (attribute) {
    case HSA_REGION_INFO_BASE:
      *((uintptr_t*)value) =
          static_cast<uintptr_t>(mem_props_.VirtualBaseAddress);
      break;
    case HSA_REGION_INFO_SIZE:
      *((size_t*)value) = static_cast<size_t>(mem_props_.SizeInBytes);
      break;
    case HSA_REGION_INFO_AGENT:
      *((hsa_agent_t*)value) = core::Agent::Convert(agent());
      break;
    case HSA_REGION_INFO_FLAGS:
      if (agent()->device_type() == core::Agent::DeviceType::kAmdGpuDevice) {
        // TODO: confirm that all segments (system, gpu local, lds) are
        // cached in GPU L1 dcache.
        uint32_t val = (uint32_t)HSA_REGION_FLAG_CACHED_L1;
        switch (mem_props_.HeapType) {
          case HSA_HEAPTYPE_SYSTEM:
            // TODO: confirm that default aperture is not cached in L2.
            val |= (uint32_t)(HSA_REGION_FLAG_KERNARG);
            break;
          case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
          case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
            // TODO: confirm that gpuvm aperture is cached in L2.
            val |= (uint32_t)HSA_REGION_FLAG_CACHED_L2;
            break;
          default:
            // TODO: confirm that LDS is not cached in L2.
            break;
        }
        *((hsa_region_flag_t*)value) = (hsa_region_flag_t)val;
      } else if (agent()->device_type() == core::Agent::kAmdCpuDevice) {
        const size_t num_caches = ((amd::CpuAgent*)agent())->num_cache();

        uint32_t val = 0;
        switch (num_caches) {
          case 4:
            val |= (uint32_t)HSA_REGION_FLAG_CACHED_L4;
          case 3:
            val |= (uint32_t)HSA_REGION_FLAG_CACHED_L3;
          case 2:
            val |= (uint32_t)HSA_REGION_FLAG_CACHED_L2;
          default:
            val |= (uint32_t)HSA_REGION_FLAG_CACHED_L1;
            break;
        }
        *((hsa_region_flag_t*)value) = (hsa_region_flag_t)val;
      } else
        assert(false && "MemoryRegion contains an invalid component.");
      break;
    case HSA_REGION_INFO_SEGMENT:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((hsa_segment_t*)value) = HSA_SEGMENT_GLOBAL;
          break;
        case HSA_HEAPTYPE_GPU_LDS:
          *((hsa_segment_t*)value) = HSA_SEGMENT_GROUP;
          break;
        case HSA_HEAPTYPE_GPU_SCRATCH:
          *((hsa_segment_t*)value) = HSA_SEGMENT_SPILL;
          break;
        default:
          assert(false &&
                 "Memory region should only be global, group, or scratch");
          break;
      }
      break;
    case HSA_REGION_INFO_ALLOC_MAX_SIZE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          // TODO: Limiting to 128MB for now.
          *((size_t*)value) = 128 * 1024 * 1024;
          ;
          break;
        case HSA_HEAPTYPE_SYSTEM:
          *((size_t*)value) = static_cast<size_t>(mem_props_.SizeInBytes);
          break;
        default:
          *((size_t*)value) = 0;
      }
      break;
    case HSA_REGION_INFO_ALLOC_GRANULE:
      // TODO: remove the hardcoded value.
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = 4096;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_ALLOC_ALIGNMENT:
      // TODO: remove the hardcoded value.
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = 4096;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_BANDWIDTH:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          // TODO: Get info on garlic/onion bandwidth ratio.
          bandwidth_multiplier = 2;
        case HSA_HEAPTYPE_SYSTEM:
          // TODO: Not enough information about dual/triple channel, DDR.
          *((uint32_t*)value) =
              bandwidth_multiplier *
              CalculateDDR3Bandwidth(mem_props_.MemoryClockMax,
                                     mem_props_.Width);
          break;
        case HSA_HEAPTYPE_GPU_LDS:
          //TODO: hardcode for now at 720MHz core clock
			*((uint32_t*)value) = 754975;
          break;
        default:
          *((uint32_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_NODE:
      MemoryRegion::agent()->GetInfo(HSA_AGENT_INFO_NODE, value);
      break;
    default:
      switch ((hsa_amd_region_info_t)attribute) {
        case HSA_EXT_REGION_INFO_HOST_ACCESS:
          *((bool*)value) =
              (mem_props_.HeapType == HSA_HEAPTYPE_SYSTEM) ? true : false;
          break;
        default:
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
          break;
      }
      break;
  }
  return HSA_STATUS_SUCCESS;
}

}  // namespace
