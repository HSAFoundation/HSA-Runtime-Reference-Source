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

#include <algorithm>

#include "core/inc/runtime.h"
#include "core/inc/memory_database.h"
#include "core/inc/amd_cpu_agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/thunk.h"
#include "core/util/utils.h"

namespace amd {
/// @brief Allocate agent accessible memory (system / local memory).
static void* AllocateKfdMemory(const HsaMemFlags& flag, HSAuint32 node_id,
                               size_t size) {
  void* ret = NULL;
  const HSAKMT_STATUS status = hsaKmtAllocMemory(node_id, size, flag, &ret);
  if (status != HSAKMT_STATUS_SUCCESS) return NULL;

  core::Runtime::runtime_singleton_->Register(ret, size, false);

  return ret;
}

/// @brief Free agent accessible memory (system / local memory).
static void FreeKfdMemory(void* ptr, size_t size) {
  if (ptr == NULL || size == 0) {
    return;
  }

  // Completely deregister ptr (could be two references on the registration due
  // to an explicit registration call)
  while (core::Runtime::runtime_singleton_->Deregister(ptr))
    ;

  HSAKMT_STATUS status = hsaKmtFreeMemory(ptr, size);
  assert(status == HSAKMT_STATUS_SUCCESS);
}

static bool MakeKfdMemoryResident(void* ptr, size_t size) {
  HSAuint64 alternate_va;
  HSAKMT_STATUS status = hsaKmtMapMemoryToGPU(ptr, size, &alternate_va);
  return (status == HSAKMT_STATUS_SUCCESS);
}

static void MakeKfdMemoryUnresident(void* ptr) { hsaKmtUnmapMemoryToGPU(ptr); }

MemoryRegion::MemoryRegion(bool fine_grain, uint32_t node_id,
                           const HsaMemoryProperties& mem_props)
    : core::MemoryRegion(fine_grain),
      node_id_(node_id),
      mem_props_(mem_props),
      max_single_alloc_size_(0),
      virtual_size_(0) {
  virtual_size_ = GetPhysicalSize();

  mem_flag_.Value = 0;

  if (IsLocalMemory()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = 0;
    mem_flag_.ui32.NonPaged = 1;

    char* char_end = NULL;
    HSAuint64 max_alloc_size = static_cast<HSAuint64>(strtoull(
        os::GetEnvVar("HSA_LOCAL_MEMORY_MAX_ALLOC").c_str(), &char_end, 10));
    max_alloc_size = std::max(max_alloc_size, GetPhysicalSize() / 4);
    max_alloc_size = std::min(max_alloc_size, GetPhysicalSize());

    max_single_alloc_size_ =
        AlignDown(static_cast<size_t>(max_alloc_size), kPageSize_);

    static const HSAuint64 kGpuVmSize = (1ULL << 40);
    virtual_size_ = kGpuVmSize;
  } else if (IsSystem()) {
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = 1;
    mem_flag_.ui32.CachePolicy = HSA_CACHING_CACHED;

    if (fine_grain) {
      max_single_alloc_size_ =
          AlignDown(static_cast<size_t>(GetPhysicalSize()), kPageSize_);

      virtual_size_ = os::GetUserModeVirtualMemorySize();
    } else {
      max_single_alloc_size_ =
          AlignDown(static_cast<size_t>(GetPhysicalSize() / 4), kPageSize_);

      virtual_size_ = GetPhysicalSize();
    }
  }

  assert(GetVirtualSize() != 0);
  assert(GetPhysicalSize() <= GetVirtualSize());
  assert(IsMultipleOf(max_single_alloc_size_, kPageSize_));
}

MemoryRegion::~MemoryRegion() {}

hsa_status_t MemoryRegion::Allocate(size_t size, void** address) const {
  if (address == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  *address = amd::AllocateKfdMemory(mem_flag_, node_id_, size);

  if (*address != NULL) {
    if (fine_grain()) {
      amd::MakeKfdMemoryResident(*address, size);
    } else {
      // TODO: remove immediate pinning on coarse grain memory when HSA API to
      // explicitly unpin memory is available.
      if (!amd::MakeKfdMemoryResident(*address, size)) {
        amd::FreeKfdMemory(*address, size);
        *address = NULL;
        return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
      }
    }

    return HSA_STATUS_SUCCESS;
  }

  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
}

hsa_status_t MemoryRegion::Free(void* address, size_t size) const {
  amd::MakeKfdMemoryUnresident(address);

  amd::FreeKfdMemory(address, size);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::GetInfo(hsa_region_info_t attribute,
                                   void* value) const {
  switch (attribute) {
    case HSA_REGION_INFO_SEGMENT:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((hsa_region_segment_t*)value) = HSA_REGION_SEGMENT_GLOBAL;
          break;
        case HSA_HEAPTYPE_GPU_LDS:
          *((hsa_region_segment_t*)value) = HSA_REGION_SEGMENT_GROUP;
          break;
        default:
          assert(false && "Memory region should only be global, group");
          break;
      }
      break;
    case HSA_REGION_INFO_GLOBAL_FLAGS:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
          *((uint32_t*)value) =
              (HSA_REGION_GLOBAL_FLAG_KERNARG |
               ((fine_grain()) ? HSA_REGION_GLOBAL_FLAG_FINE_GRAINED
                               : HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED));
          break;
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((uint32_t*)value) = HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED;
          break;
        default:
          *((uint32_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_SIZE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          // TODO: report the actual physical size of local memory until API to
          // explicitly unpin memory is available.
          *((size_t*)value) = static_cast<size_t>(GetPhysicalSize());
          break;
        default:
          *((size_t*)value) = static_cast<size_t>(GetVirtualSize());
          break;
      }
      break;
    case HSA_REGION_INFO_ALLOC_MAX_SIZE:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
        case HSA_HEAPTYPE_SYSTEM:
          *((size_t*)value) = max_single_alloc_size_;
          break;
        default:
          *((size_t*)value) = 0;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_ALLOWED:
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((bool*)value) = true;
          break;
        default:
          *((bool*)value) = false;
          break;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_GRANULE:
      // TODO: remove the hardcoded value.
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = kPageSize_;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    case HSA_REGION_INFO_RUNTIME_ALLOC_ALIGNMENT:
      // TODO: remove the hardcoded value.
      switch (mem_props_.HeapType) {
        case HSA_HEAPTYPE_SYSTEM:
        case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
        case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          *((size_t*)value) = kPageSize_;
          break;
        default:
          *((size_t*)value) = 0;
          break;
      }
      break;
    default:
      switch ((hsa_amd_region_info_t)attribute) {
        case HSA_AMD_REGION_INFO_HOST_ACCESSIBLE:
          *((bool*)value) =
              (mem_props_.HeapType == HSA_HEAPTYPE_SYSTEM) ? true : false;
          break;
        case HSA_AMD_REGION_INFO_BASE:
          *((void**)value) = reinterpret_cast<void*>(GetBaseAddress());
          break;
        default:
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
          break;
      }
      break;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t MemoryRegion::AssignAgent(void* ptr, size_t size,
                                       const core::Agent& agent,
                                       hsa_access_permission_t access) const {
  if (fine_grain()) {
    return HSA_STATUS_SUCCESS;
  }

  if (std::find(agent.regions().begin(), agent.regions().end(), this) ==
      agent.regions().end()) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  HSAuint64 u_ptr = reinterpret_cast<HSAuint64>(ptr);
  if (u_ptr >= GetBaseAddress() &&
      u_ptr < (GetBaseAddress() + GetVirtualSize())) {
    // TODO: only support agent allocation buffer.

    // TODO: commented until API to explicitly unpin memory is available.
    // if (!MakeKfdMemoryResident(ptr, size)) {
    //  return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    //}

    return HSA_STATUS_SUCCESS;
  } else {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  return HSA_STATUS_SUCCESS;
}

}  // namespace
