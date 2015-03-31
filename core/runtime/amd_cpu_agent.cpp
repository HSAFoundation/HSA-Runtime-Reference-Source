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

#include "core/inc/amd_cpu_agent.h"

#include <algorithm>
#include <cstring>

#include "core/inc/amd_memory_region.h"
#include "core/inc/host_queue.h"

namespace amd {
CpuAgent::CpuAgent(HSAuint32 node, const HsaNodeProperties& node_props,
                   const std::vector<HsaCacheProperties>& cache_props)
    : core::Agent(kAmdCpuDevice),
      node_id_(node),
      properties_(node_props),
      cache_props_(cache_props) {}

CpuAgent::~CpuAgent() {
  std::for_each(core::Agent::regions_.begin(), core::Agent::regions_.end(),
                DeleteObject());
  core::Agent::regions_.clear();
}

void CpuAgent::RegisterMemoryProperties(
    const HsaMemoryProperties properties) {
  assert((properties.HeapType != HSA_HEAPTYPE_GPU_GDS) &&
         (properties.HeapType != HSA_HEAPTYPE_GPU_SCRATCH) &&
         ("Memory region should only be global or group"));
  core::Agent::regions_.push_back(new MemoryRegion(*this, properties));
}

hsa_status_t CpuAgent::IterateRegion(
    hsa_status_t (*callback)(hsa_region_t region, void* data),
    void* data) const {
  const size_t num_mems = regions().size();

  for (size_t j = 0; j < num_mems; ++j) {
    hsa_region_t region = core::MemoryRegion::Convert(regions()[j]);
    hsa_status_t status = callback(region, data);
    if (status != HSA_STATUS_SUCCESS) {
      return status;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t CpuAgent::GetInfo(hsa_agent_info_t attribute, void* value) const {
  const size_t kNameSize = 64;  // agent, and vendor name size limit
  switch (attribute) {
    case HSA_AGENT_INFO_NAME:
      // TODO: hardcode for now.
      std::memset(value, 0, kNameSize);
      std::memcpy(value, "Kaveri CPU", sizeof("Kaveri CPU"));
      break;
    case HSA_AGENT_INFO_VENDOR_NAME:
      std::memset(value, 0, kNameSize);
      std::memcpy(value, "AMD", sizeof("AMD"));
      break;
    case HSA_AGENT_INFO_FEATURE:
      *((hsa_agent_feature_t*)value) = HSA_AGENT_FEATURE_AGENT_DISPATCH;
      break;
    case HSA_AGENT_INFO_WAVEFRONT_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_WORKGROUP_MAX_DIM:
      std::memset(value, 0, sizeof(uint16_t) * 3);
      break;
    case HSA_AGENT_INFO_WORKGROUP_MAX_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_GRID_MAX_DIM:
      std::memset(value, 0, sizeof(hsa_dim3_t));
      break;
    case HSA_AGENT_INFO_GRID_MAX_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_FBARRIER_MAX_SIZE:
      // TODO: ?
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_QUEUES_MAX:
      *((uint32_t*)value) = 1024;
      break;
    case HSA_AGENT_INFO_QUEUE_MAX_SIZE:
      *((uint32_t*)value) = 0x80000000;
      break;
    case HSA_AGENT_INFO_QUEUE_TYPE:
      *((hsa_queue_type_t*)value) = HSA_QUEUE_TYPE_MULTI;
      break;
    case HSA_AGENT_INFO_NODE:
      *((uint32_t*)value) = properties_.LocationId;
      break;
    case HSA_AGENT_INFO_DEVICE:
      *((hsa_device_type_t*)value) = HSA_DEVICE_TYPE_CPU;
      break;
    case HSA_AGENT_INFO_CACHE_SIZE: {
      std::memset(value, 0, sizeof(uint32_t) * 4);

      assert(cache_props_.size() > 0 && "CPU cache info missing.");
      const size_t num_cache = cache_props_.size();
      for (size_t i = 0; i < num_cache; ++i) {
        const uint32_t line_level = cache_props_[i].CacheLevel;
        ((uint32_t*)value)[line_level - 1] = cache_props_[i].CacheSize * 1024;
      }
    } break;
    case HSA_EXT_AGENT_INFO_IMAGE1D_MAX_DIM:
      std::memset(value, 0, sizeof(hsa_dim3_t));
      break;
    case HSA_EXT_AGENT_INFO_IMAGE2D_MAX_DIM:
      std::memset(value, 0, sizeof(hsa_dim3_t));
      break;
    case HSA_EXT_AGENT_INFO_IMAGE3D_MAX_DIM:
      std::memset(value, 0, sizeof(hsa_dim3_t));
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_ARRAY_MAX_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_RD_MAX:
      *((uint32_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_RDWR_MAX:
      *((uint32_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_SAMPLER_MAX:
      *((uint32_t*)value) = 0;
      break;
    default:
      switch ((hsa_amd_agent_info_t)attribute) {
        case HSA_EXT_AGENT_INFO_DEVICE_ID:
          *((uint32_t*)value) = properties_.DeviceId;
          break;
        case HSA_EXT_AGENT_INFO_CACHELINE_SIZE:
          // TODO: hardcode for now.
          *((uint32_t*)value) = 64;
          break;
        case HSA_EXT_AGENT_INFO_COMPUTE_UNIT_COUNT:
          *((uint32_t*)value) = properties_.NumCPUCores;
          break;
        case HSA_EXT_AGENT_INFO_MAX_CLOCK_FREQUENCY:
          *((uint32_t*)value) = properties_.MaxEngineClockMhzCCompute;
          break;
        default:
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
          break;
      }
      break;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t CpuAgent::QueueCreate(size_t size, hsa_queue_type_t type,
                                   core::HsaEventCallback callback,
                                   const hsa_queue_t* service_queue,
                                   core::Queue** queue) {
  if (!IsPowerOfTwo(size))
    return HSA_STATUS_ERROR;  // AQL queues must be a power of two in length.

  core::HostQueue* host_queue = new core::HostQueue(uint32_t(size));
  if (!host_queue->active()) {
    delete queue;
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }
  *queue = host_queue;
  return HSA_STATUS_SUCCESS;
}

}  // namespace
