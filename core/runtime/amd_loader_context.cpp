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
#include "core/inc/amd_loader_context.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include "inc/hsa_ext_amd.h"
#include <utility>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

#define alc_aligned_alloc(l, a) _mm_malloc(l, a)
#define alc_aligned_free(p) _mm_free(p)
#else
#include <sys/mman.h>

#define alc_aligned_alloc(l, a) aligned_alloc(a, l)
#define alc_aligned_free(p) free(p)
#endif // _WIN32 || _WIN64

namespace amd {

//===----------------------------------------------------------------------===//
// LoaderContext - Public.                                                    //
//===----------------------------------------------------------------------===//

hsa_isa_t LoaderContext::IsaFromName(const char *name) {
  assert(name);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  hsa_isa_t isa_handle; isa_handle.handle = 0;

  hsa_status = hsa_isa_from_name(name, &isa_handle);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    isa_handle.handle = 0;
    return isa_handle;
  }

  return isa_handle;
}

bool LoaderContext::IsaSupportedByAgent(hsa_agent_t agent, hsa_isa_t code_object_isa)
{
  assert(agent.handle);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  hsa_isa_t agent_isa; agent_isa.handle = 0;

  hsa_status = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &agent_isa);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    return false;
  }

  bool result = false;

  hsa_status = hsa_isa_compatible(code_object_isa, agent_isa, &result);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    return false;
  }

  return result;
}

void* LoaderContext::SegmentAlloc(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, size_t size, size_t align, bool zero) {
  assert(size);
  assert(align);
  switch (segment) {
  case AMDGPU_HSA_SEGMENT_GLOBAL_PROGRAM:
    return ProgramGlobalAlloc(size, align, zero);
  case AMDGPU_HSA_SEGMENT_GLOBAL_AGENT:
  case AMDGPU_HSA_SEGMENT_READONLY_AGENT:
    return AgentAlloc(agent, size, align, zero);
  case AMDGPU_HSA_SEGMENT_CODE_AGENT:
    return KernelCodeAlloc(agent, size, align, zero);
  default:
    assert(false); return 0;
  }
}

bool LoaderContext::SegmentCopy(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* dst, size_t offset, const void* src, size_t size)
{
  void* odst = (char*) dst + offset;
  switch (segment) {
  case AMDGPU_HSA_SEGMENT_GLOBAL_PROGRAM:
    return ProgramGlobalCopy(odst, src, size);
  case AMDGPU_HSA_SEGMENT_GLOBAL_AGENT:
  case AMDGPU_HSA_SEGMENT_READONLY_AGENT:
    return AgentCopy(odst, src, size);
  case AMDGPU_HSA_SEGMENT_CODE_AGENT:
    return KernelCodeCopy(odst, src, size);
  default:
    assert(false); return false;
  }
}

void LoaderContext::SegmentFree(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t size)
{
  switch (segment) {
  case AMDGPU_HSA_SEGMENT_GLOBAL_PROGRAM: ProgramGlobalFree(seg, size); break;
  case AMDGPU_HSA_SEGMENT_GLOBAL_AGENT: 
  case AMDGPU_HSA_SEGMENT_READONLY_AGENT: AgentFree(seg, size); break;
  case AMDGPU_HSA_SEGMENT_CODE_AGENT: KernelCodeFree(seg, size); break;
  default:
    assert(false); return;
  }
}

void* LoaderContext::SegmentAddress(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t offset)
{
  return (char*) seg + offset;
}

void* LoaderContext::ProgramGlobalAlloc(size_t size, size_t align, bool zero) {
  assert(size);
  assert(align);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  void *result = NULL;

  result = alc_aligned_alloc(size, align);
  if (!result) {
    return NULL;
  }

  hsa_status = hsa_memory_register(result, size);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    alc_aligned_free(result);
    return NULL;
  }

  return zero ? memset(result, 0x0, size) : result;
}

bool LoaderContext::ProgramGlobalCopy(void *dst, const void *src, size_t size) {
  assert(dst);
  assert(src);
  assert(size);

  memcpy(dst, src, size);
  return true;
}

void LoaderContext::ProgramGlobalFree(void *ptr, size_t size) {
  assert(ptr);
  assert(size);

  hsa_memory_deregister(ptr, size);
  alc_aligned_free(ptr);
}

void* LoaderContext::KernelCodeAlloc(hsa_agent_t agent, size_t size, size_t align, bool zero) {
  assert(agent.handle);
  assert(size);
  assert(align);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  void *result = NULL;

#if defined(_WIN32) || defined(_WIN64)
  result = (void*)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
  result = (void*)mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif
  if (!result) {
    return NULL;
  }

  hsa_status = hsa_memory_register(result, size);
  if (HSA_STATUS_SUCCESS != hsa_status) {
#if defined(_WIN32) || defined(_WIN64)
    VirtualFree(result, size, MEM_DECOMMIT);
    VirtualFree(result, 0, MEM_RELEASE);
#else
    munmap(result, size);
#endif
    return NULL;
  }

  return zero ? memset(result, 0x0, size) : result;
}

bool LoaderContext::KernelCodeCopy(void *dst, const void *src, size_t size) {
  assert(dst);
  assert(src);
  assert(size);

  memcpy(dst, src, size);
  return true;
}

void LoaderContext::KernelCodeFree(void *ptr, size_t size) {
  assert(ptr);
  assert(size);

  hsa_memory_deregister(ptr, size);

#if defined(_WIN32) || defined(_WIN64)
  VirtualFree(ptr, size, MEM_DECOMMIT);
  VirtualFree(ptr, 0, MEM_RELEASE);
#else
  munmap(ptr, size);
#endif
}

bool LoaderContext::ImageExtensionSupported() {
  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  bool result = false;

  hsa_status = hsa_system_extension_supported(HSA_EXTENSION_IMAGES, 1, 0, &result);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    return false;
  }

  return result;
}

hsa_status_t LoaderContext::ImageCreate(hsa_agent_t agent,
                                        hsa_access_permission_t image_permission,
                                        const hsa_ext_image_descriptor_t *image_descriptor,
                                        const void *image_data,
                                        hsa_ext_image_t *image_handle) {
  assert(agent.handle);
  assert(image_descriptor);
  assert(image_data);
  assert(image_handle);

  assert(ImageExtensionSupported());

  return hsa_ext_image_create(agent, image_descriptor, image_data, image_permission, image_handle);
}

hsa_status_t LoaderContext::ImageDestroy(hsa_agent_t agent, hsa_ext_image_t image_handle) {
  assert(agent.handle);
  assert(image_handle.handle);

  assert(ImageExtensionSupported());

  return hsa_ext_image_destroy(agent, image_handle);
}

hsa_status_t LoaderContext::SamplerCreate(hsa_agent_t agent,
                                          const hsa_ext_sampler_descriptor_t *sampler_descriptor,
                                          hsa_ext_sampler_t *sampler_handle) {
  assert(agent.handle);
  assert(sampler_descriptor);
  assert(sampler_handle);

  assert(ImageExtensionSupported());

  return hsa_ext_sampler_create(agent, sampler_descriptor, sampler_handle);
}

hsa_status_t LoaderContext::SamplerDestroy(hsa_agent_t agent, hsa_ext_sampler_t sampler_handle) {
  assert(agent.handle);
  assert(sampler_handle.handle);

  assert(ImageExtensionSupported());

  return hsa_ext_sampler_destroy(agent, sampler_handle);
}

void LoaderContext::Reset() {
  std::lock_guard<std::mutex> lock(agent2region_mutex_);
  agent2region_.clear();
}

//===----------------------------------------------------------------------===//
// LoaderContext - Private.                                                   //
//===----------------------------------------------------------------------===//

hsa_status_t LoaderContext::FindRegion(hsa_region_segment_t segment, hsa_region_t region, void *data) {
  assert(data);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  bool host = false;

  hsa_status = hsa_region_get_info(
    region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    return hsa_status;
  }

  if (host) {
    hsa_region_segment_t current_segment;
    hsa_status = hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &current_segment);
    if (HSA_STATUS_SUCCESS != hsa_status) {
      return hsa_status;
    }
    if (segment == current_segment) {
      *((hsa_region_t*)data) = region;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t LoaderContext::FindGlobalRegion(hsa_region_t region, void *data) {
  return FindRegion(HSA_REGION_SEGMENT_GLOBAL, region, data);
}

hsa_status_t LoaderContext::FindReadonlyRegion(hsa_region_t region, void *data) {
  return FindRegion(HSA_REGION_SEGMENT_READONLY, region, data);
}

void* LoaderContext::AgentAlloc(hsa_agent_t agent, size_t size, size_t align, bool zero) {
  assert(agent.handle);
  assert(size);
  assert(align);

  hsa_status_t hsa_status = HSA_STATUS_SUCCESS;
  void *result = NULL;

  Agent2RegionMap::iterator used_region = agent2region_.end();
  {
    std::lock_guard<std::mutex> lock(agent2region_mutex_);

    used_region = agent2region_.find(agent);
    if (used_region == agent2region_.end()) {
      hsa_region_t agent_region = {0};
      hsa_status = hsa_agent_iterate_regions(agent, FindGlobalRegion, &agent_region);
      if (HSA_STATUS_SUCCESS != hsa_status) {
        return NULL;
      }

      used_region = agent2region_.insert(used_region, std::make_pair(agent, agent_region));
    }
  }
  assert(used_region != agent2region_.end());

  assert(used_region->first.handle == agent.handle);
  assert(used_region->second.handle);

  hsa_status = hsa_memory_allocate(used_region->second, size, &result);
  if (HSA_STATUS_SUCCESS != hsa_status) {
    return NULL;
  }

  assert(result);

  // @todo(runtime): need more efficient way of allocating zero-initialized
  // memory.
  if (zero) {
    void *zero_initialized = calloc(size, 1);
    if (!zero_initialized) {
      hsa_memory_free(result);
      return NULL;
    }

    hsa_status = hsa_memory_copy(result, zero_initialized, size);
    if (HSA_STATUS_SUCCESS != hsa_status) {
      hsa_memory_free(result);
      free(zero_initialized);
      return NULL;
    }

    free(zero_initialized);
  }

  return result;
}

bool LoaderContext::AgentCopy(void *dst, const void *src, size_t size) {
  assert(dst);
  assert(src);
  assert(size);

  return HSA_STATUS_SUCCESS == hsa_memory_copy(dst, src, size);
}

void LoaderContext::AgentFree(void *ptr, size_t size) {
  assert(ptr);
  assert(size);

  hsa_memory_free(ptr);
}

} // namespace amd
