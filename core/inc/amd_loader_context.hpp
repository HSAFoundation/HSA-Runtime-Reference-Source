////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation (if
// any) (collectively, the "Materials") pursuant to the terms and conditions of
// the Software License Agreement included with the Materials. If you do not
// have a copy of the Software License Agreement, contact your AMD
// representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND. AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE. THE ENTIRE RISK ASSOCIATED WITH THE USE
// OF THE SOFTWARE IS ASSUMED BY YOU. Some jurisdictions do not allow the
// exclusion of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION: AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD. You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor. Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef HSA_RUNTIME_CORE_INC_AMD_LOADER_CONTEXT_HPP
#define HSA_RUNTIME_CORE_INC_AMD_LOADER_CONTEXT_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include "core/inc/amd_hsa_loader.hpp"
#include "inc/hsa.h"
#include "inc/hsa_ext_image.h"
#include <mutex>
#include <unordered_map>

namespace amd {

//===----------------------------------------------------------------------===//
// Agent2RegionMap.                                                           //
//===----------------------------------------------------------------------===//

struct AgentHash {
  size_t operator()(const hsa_agent_t &agent) const {
    return std::hash<uint64_t>()(agent.handle);
  }
};

struct AgentCompare {
  bool operator()(const hsa_agent_t &left, const hsa_agent_t &right) const {
    return left.handle == right.handle;
  }
};

typedef std::unordered_map<hsa_agent_t, hsa_region_t, AgentHash, AgentCompare> Agent2RegionMap;

//===----------------------------------------------------------------------===//
// LoaderContext.                                                             //
//===----------------------------------------------------------------------===//

class LoaderContext final: public hsa::loader::Context {
public:
  LoaderContext(): hsa::loader::Context() {}

  ~LoaderContext() {}

  hsa_isa_t IsaFromName(const char *name) override;

  bool IsaSupportedByAgent(hsa_agent_t agent, hsa_isa_t code_object_isa) override;

  void* SegmentAlloc(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, size_t size, size_t align, bool zero) override;

  bool SegmentCopy(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* dst, size_t offset, const void* src, size_t size) override;

  void SegmentFree(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t size = 0) override;

  void* SegmentAddress(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t offset) override;

  bool ImageExtensionSupported();

  hsa_status_t ImageCreate(
    hsa_agent_t agent,
    hsa_access_permission_t image_permission,
    const hsa_ext_image_descriptor_t *image_descriptor,
    const void *image_data,
    hsa_ext_image_t *image_handle);

  hsa_status_t ImageDestroy(hsa_agent_t agent, hsa_ext_image_t image_handle);

  hsa_status_t SamplerCreate(
    hsa_agent_t agent,
    const hsa_ext_sampler_descriptor_t *sampler_descriptor,
    hsa_ext_sampler_t *sampler_handle);

  hsa_status_t SamplerDestroy(hsa_agent_t agent, hsa_ext_sampler_t sampler_handle);

  void Reset();

private:
  LoaderContext(const LoaderContext &lc);
  LoaderContext& operator=(const LoaderContext &lc);

  static hsa_status_t FindRegion(hsa_region_segment_t segment, hsa_region_t region, void *data);
  static hsa_status_t FindGlobalRegion(hsa_region_t region, void *data);
  static hsa_status_t FindReadonlyRegion(hsa_region_t region, void *data);

  void* ProgramGlobalAlloc(size_t size, size_t align, bool zero);
  bool ProgramGlobalCopy(void *dst, const void *src, size_t size);
  void ProgramGlobalFree(void *ptr, size_t size);
  void* AgentAlloc(hsa_agent_t agent, size_t size, size_t align, bool zero);
  bool AgentCopy(void *dst, const void *src, size_t size);
  void AgentFree(void *ptr, size_t size);
  void* KernelCodeAlloc(hsa_agent_t agent, size_t size, size_t align, bool zero);
  bool KernelCodeCopy(void *dst, const void *src, size_t size);
  void KernelCodeFree(void *ptr, size_t size);

  std::mutex agent2region_mutex_;
  Agent2RegionMap agent2region_;
};

} // namespace amd

#endif // HSA_RUNTIME_CORE_INC_AMD_LOADER_CONTEXT_HPP
