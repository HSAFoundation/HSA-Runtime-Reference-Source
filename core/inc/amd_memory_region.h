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

// AMD specific HSA backend.

#ifndef HSA_RUNTIME_CORE_INC_AMD_MEMORY_REGION_H_
#define HSA_RUNTIME_CORE_INC_AMD_MEMORY_REGION_H_

#include "core/inc/agent.h"
#include "core/inc/memory_region.h"
#include "core/inc/thunk.h"

namespace amd {
class MemoryRegion : public core::MemoryRegion {
 public:
  MemoryRegion(bool fine_grain, const core::Agent& owner,
               const HsaMemoryProperties& mem_props);

  ~MemoryRegion();

  hsa_status_t Allocate(size_t size, void** address) const;

  hsa_status_t Free(void* address, size_t size) const;

  hsa_status_t GetInfo(hsa_region_info_t attribute, void* value) const;

  HSAuint64 GetBaseAddress() const { return mem_props_.VirtualBaseAddress; }

  HSAuint64 GetPhysicalSize() const { return mem_props_.SizeInBytes; }

  HSAuint64 GetVirtualSize() const { return virtual_size_; }

  hsa_status_t AssignAgent(void* ptr, size_t size, const core::Agent& agent,
                           hsa_access_permission_t access) const;

  __forceinline bool IsLocalMemory() const {
    return ((mem_props_.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE) ||
            (mem_props_.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC));
  }

  __forceinline bool IsSystem() const {
    return mem_props_.HeapType == HSA_HEAPTYPE_SYSTEM;
  }

  __forceinline bool IsLDS() const {
    return mem_props_.HeapType == HSA_HEAPTYPE_GPU_LDS;
  }

  __forceinline bool IsGDS() const {
    return mem_props_.HeapType == HSA_HEAPTYPE_GPU_GDS;
  }

  __forceinline bool IsScratch() const {
    return mem_props_.HeapType == HSA_HEAPTYPE_GPU_SCRATCH;
  }

  __forceinline const core::Agent* owner() const { return owner_; }

 private:
  const core::Agent* owner_;

  const HsaMemoryProperties mem_props_;

  HsaMemFlags mem_flag_;

  size_t max_single_alloc_size_;

  HSAuint64 virtual_size_;

  static const size_t kPageSize_ = 4096;
};
}  // namespace

#endif  // header guard
