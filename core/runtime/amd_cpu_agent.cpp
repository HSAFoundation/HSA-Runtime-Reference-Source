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

#include "inc/hsa_ext_image.h"

namespace amd {
CpuAgent::CpuAgent(HSAuint32 node, const HsaNodeProperties& node_props,
                   const std::vector<HsaCacheProperties>& cache_props)
    : core::Agent(kAmdCpuDevice),
      node_id_(node),
      properties_(node_props),
      cache_props_(cache_props) {}

CpuAgent::~CpuAgent() { regions_.clear(); }

void CpuAgent::RegisterMemoryProperties(core::MemoryRegion& region) {
  MemoryRegion* amd_region = reinterpret_cast<MemoryRegion*>(&region);

  assert((amd_region->IsSystem()) &&
         ("Memory region should only be system memory"));

  regions_.push_back(amd_region);
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

  const size_t attribute_u = static_cast<size_t>(attribute);
  switch (attribute_u) {
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
      *((hsa_agent_feature_t*)value) = static_cast<hsa_agent_feature_t>(0);
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
      // TODO: validate if this is true.
      *((hsa_default_float_rounding_mode_t*)value) =
          HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR;
      break;
    case HSA_AGENT_INFO_FAST_F16_OPERATION:
      // TODO: validate if this is trye.
      *((bool*)value) = false;
      break;
    case HSA_AGENT_INFO_PROFILE:
      *((hsa_profile_t*)value) = HSA_PROFILE_FULL;
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
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_QUEUE_MIN_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_QUEUE_MAX_SIZE:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AGENT_INFO_QUEUE_TYPE:
      *((hsa_queue_type_t*)value) = static_cast<hsa_queue_type_t>(0);
      break;
    case HSA_AGENT_INFO_NODE:
      // TODO: associate with OS NUMA support (numactl / GetNumaProcessorNode).
      *((uint32_t*)value) = node_id_;
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
    case HSA_AGENT_INFO_ISA:
      ((hsa_isa_t*)value)->handle = 0;
      break;
    case HSA_AGENT_INFO_EXTENSIONS:
      memset(value, 0, sizeof(uint8_t) * 128);
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
      *((uint32_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_2D_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DA_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DDEPTH_MAX_ELEMENTS:
    case HSA_EXT_AGENT_INFO_IMAGE_2DADEPTH_MAX_ELEMENTS:
      memset(value, 0, sizeof(uint32_t) * 2);
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_3D_MAX_ELEMENTS:
      memset(value, 0, sizeof(uint32_t) * 3);
      break;
    case HSA_EXT_AGENT_INFO_IMAGE_ARRAY_MAX_LAYERS:
      *((uint32_t*)value) = 0;
      break;
    case HSA_EXT_AGENT_INFO_MAX_IMAGE_RD_HANDLES:
    case HSA_EXT_AGENT_INFO_MAX_IMAGE_RORW_HANDLES:
    case HSA_EXT_AGENT_INFO_MAX_SAMPLER_HANDLERS:
      *((uint32_t*)value) = 0;
      break;
    case HSA_AMD_AGENT_INFO_CHIP_ID:
      *((uint32_t*)value) = properties_.DeviceId;
      break;
    case HSA_AMD_AGENT_INFO_CACHELINE_SIZE:
      // TODO: hardcode for now.
      *((uint32_t*)value) = 64;
      break;
    case HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT:
      *((uint32_t*)value) = properties_.NumCPUCores;
      break;
    case HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY:
      *((uint32_t*)value) = properties_.MaxEngineClockMhzCCompute;
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

hsa_status_t CpuAgent::QueueCreate(size_t size, hsa_queue_type_t queue_type,
                                   core::HsaEventCallback event_callback,
                                   void* data, uint32_t private_segment_size,
                                   uint32_t group_segment_size,
                                   core::Queue** queue) {
  // No HW AQL packet processor on CPU device.
  return HSA_STATUS_ERROR;
}

}  // namespace
