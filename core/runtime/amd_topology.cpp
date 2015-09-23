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

#include "core/inc/amd_topology.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "core/inc/runtime.h"
#include "core/inc/amd_cpu_agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/amd_dgpu_agent.h"
#include "core/inc/amd_memory_region.h"
#include "core/inc/thunk.h"
#include "core/util/utils.h"

namespace amd {
// Minimum acceptable KFD version numbers
static const uint kKfdVersionMajor = 0;
static const uint kKfdVersionMinor = 99;

CpuAgent* DiscoverCpu(HSAuint32 node_id, HsaNodeProperties& node_prop) {
  if (node_prop.NumCPUCores == 0) {
    return NULL;
  }

  // Get CPU cache information.
  std::vector<HsaCacheProperties> cache_props(node_prop.NumCaches);
  if (HSAKMT_STATUS_SUCCESS !=
      hsaKmtGetNodeCacheProperties(node_id, node_prop.CComputeIdLo,
                                   node_prop.NumCaches, &cache_props[0])) {
    cache_props.clear();
  } else {
    // Only store CPU D-cache.
    for (size_t cache_id = 0; cache_id < cache_props.size(); ++cache_id) {
      const HsaCacheType type = cache_props[cache_id].CacheType;
      if (type.ui32.CPU != 1 || type.ui32.Instruction == 1) {
        cache_props.erase(cache_props.begin() + cache_id);
        --cache_id;
      }
    }
  }

  CpuAgent* cpu = new CpuAgent(node_id, node_prop, cache_props);
  core::Runtime::runtime_singleton_->RegisterAgent(cpu);

  return cpu;
}

GpuAgent* DiscoverGpu(HSAuint32 node_id, HsaNodeProperties& node_prop) {
  if (node_prop.NumFComputeCores == 0) {
    return NULL;
  }

  // Get GPU cache information.
  // Similar to getting CPU cache but here we use FComputeIdLo.
  std::vector<HsaCacheProperties> cache_props(node_prop.NumCaches);
  if (HSAKMT_STATUS_SUCCESS !=
      hsaKmtGetNodeCacheProperties(node_id, node_prop.FComputeIdLo,
                                   node_prop.NumCaches, &cache_props[0])) {
    cache_props.clear();
  } else {
    // Only store GPU D-cache.
    for (size_t cache_id = 0; cache_id < cache_props.size(); ++cache_id) {
      const HsaCacheType type = cache_props[cache_id].CacheType;
      if (type.ui32.HSACU != 1 || type.ui32.Instruction == 1) {
        cache_props.erase(cache_props.begin() + cache_id);
        --cache_id;
      }
    }
  }

  GpuAgent* gpu = NULL;

  const bool is_apu_node = (node_prop.NumCPUCores > 0);
  if (is_apu_node) {
    gpu = new GpuAgent(node_id, node_prop, cache_props);
    gpu->current_coherency_type(HSA_AMD_COHERENCY_TYPE_COHERENT);
  } else {
    gpu = new DGpuAgent(node_id, node_prop, cache_props);
  }

  assert(gpu != NULL);
  core::Runtime::runtime_singleton_->RegisterAgent(gpu);

  // Discover memory regions.
  assert(node_prop.NumMemoryBanks > 0);
  std::vector<HsaMemoryProperties> mem_props(node_prop.NumMemoryBanks);
  if (HSAKMT_STATUS_SUCCESS ==
      hsaKmtGetNodeMemoryProperties(node_id, node_prop.NumMemoryBanks,
                                    &mem_props[0])) {
    for (uint32_t mem_idx = 0; mem_idx < node_prop.NumMemoryBanks; ++mem_idx) {
      // Ignore the one(s) with unknown size.
      if (mem_props[mem_idx].SizeInBytes == 0) {
        continue;
      }

      if (mem_props[mem_idx].HeapType == HSA_HEAPTYPE_SYSTEM) {
        if (core::Runtime::runtime_singleton_->system_region().handle == 0) {
          const bool fine_grain = (is_apu_node) ? true : false;
          MemoryRegion* system_region =
              new MemoryRegion(fine_grain, node_id, mem_props[mem_idx]);

          core::Runtime::runtime_singleton_->RegisterMemoryRegion(
              system_region);
        }
      } else {
        switch (mem_props[mem_idx].HeapType) {
          case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
          case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
          case HSA_HEAPTYPE_GPU_LDS:
          case HSA_HEAPTYPE_GPU_SCRATCH:
            if (gpu != NULL) {
              MemoryRegion* region =
                  new MemoryRegion(false, node_id, mem_props[mem_idx]);
              core::Runtime::runtime_singleton_->RegisterMemoryRegion(region);
              gpu->RegisterMemoryProperties(*(region));
            }
            break;
          default:
            continue;
        }
      }
    }
  }

  return gpu;
}

/// @brief Calls Kfd thunk to get the snapshot of the topology of the system,
/// which includes associations between, node, devices, memory and caches.
void BuildTopology() {
  HsaVersionInfo info;
  if (hsaKmtGetVersion(&info) != HSAKMT_STATUS_SUCCESS) {
    return;
  }

  if (info.KernelInterfaceMajorVersion == kKfdVersionMajor &&
      info.KernelInterfaceMinorVersion < kKfdVersionMinor) {
    return;
  }

  // Disable KFD event support when using open source KFD
  if (info.KernelInterfaceMajorVersion == 1 &&
      info.KernelInterfaceMinorVersion == 0)
    core::g_use_interrupt_wait = false;

  HsaSystemProperties props;
  hsaKmtReleaseSystemProperties();

  if (hsaKmtAcquireSystemProperties(&props) != HSAKMT_STATUS_SUCCESS) {
    return;
  }

  // Discover agents on every node in the platform.
  for (HSAuint32 node_id = 0; node_id < props.NumNodes; node_id++) {
    HsaNodeProperties node_prop = {0};
    if (hsaKmtGetNodeProperties(node_id, &node_prop) != HSAKMT_STATUS_SUCCESS) {
      continue;
    }

    const CpuAgent* cpu = DiscoverCpu(node_id, node_prop);
    const GpuAgent* gpu = DiscoverGpu(node_id, node_prop);

    assert(!(cpu == NULL && gpu == NULL));
  }

  // Create system memory region if it does not exist yet.
  if (core::Runtime::runtime_singleton_->system_region().handle == 0) {
    HsaMemoryProperties system_props;
    std::memset(&system_props, 0, sizeof(HsaMemoryProperties));

    const uintptr_t system_base = os::GetUserModeVirtualMemoryBase();
    const size_t system_physical_size = os::GetUsablePhysicalHostMemorySize();
    assert(system_physical_size != 0);

    system_props.HeapType = HSA_HEAPTYPE_SYSTEM;
    system_props.SizeInBytes = (HSAuint64)system_physical_size;
    system_props.VirtualBaseAddress = (HSAuint64)(system_base);

    MemoryRegion* system_region = new MemoryRegion(true, 0, system_props);
    core::Runtime::runtime_singleton_->RegisterMemoryRegion(system_region);
  }

  assert(core::Runtime::runtime_singleton_->system_region().handle != 0);

  // Associate all agents with system memory region.
  core::Runtime::runtime_singleton_->IterateAgent(
      [](hsa_agent_t agent, void* data) -> hsa_status_t {
        core::MemoryRegion* system_region = core::MemoryRegion::Convert(
            core::Runtime::runtime_singleton_->system_region());

        core::Agent* core_agent = core::Agent::Convert(agent);

        if (core_agent->device_type() ==
            core::Agent::DeviceType::kAmdCpuDevice) {
          reinterpret_cast<amd::CpuAgent*>(core_agent)
              ->RegisterMemoryProperties(*system_region);
        } else if (core_agent->device_type() ==
                   core::Agent::DeviceType::kAmdGpuDevice) {
          reinterpret_cast<amd::GpuAgent*>(core_agent)
              ->RegisterMemoryProperties(*system_region);
        }

        return HSA_STATUS_SUCCESS;
      },
      NULL);
}

void Load() {
  // Open KFD
  if (hsaKmtOpenKFD() != HSAKMT_STATUS_SUCCESS) return;

  // Build topology table.
  BuildTopology();
}

// Releases internal resources and unloads DLLs
void Unload() {
  hsaKmtReleaseSystemProperties();

  // Close KFD
  hsaKmtCloseKFD();
}
}  // namespace
