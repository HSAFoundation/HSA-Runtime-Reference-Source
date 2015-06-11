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
#include "core/inc/amd_memory_region.h"
#include "core/inc/thunk.h"
#include "core/util/utils.h"

namespace amd {
// Minimum acceptable KFD version numbers
static const uint kKfdVersionMajor = 0;
static const uint kKfdVersionMinor = 99;

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

  for (HSAuint32 node_id = 0; node_id < props.NumNodes; node_id++) {
    HsaNodeProperties node_prop = {0};

    if (hsaKmtGetNodeProperties(node_id, &node_prop) != HSAKMT_STATUS_SUCCESS) {
      return;
    }

    CpuAgent* cpu = NULL;
    if (node_prop.NumCPUCores > 0) {
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

      cpu = new CpuAgent(node_id, node_prop, cache_props);
      core::Runtime::runtime_singleton_->RegisterAgent(cpu);
    }

    GpuAgent* gpu = NULL;
    if (node_prop.NumFComputeCores > 0) {
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

      gpu = new GpuAgent(node_id, node_prop, cache_props);
      core::Runtime::runtime_singleton_->RegisterAgent(gpu);
    }

    uint32_t num_mem_banks = node_prop.NumMemoryBanks;
    if (num_mem_banks > 0) {
      std::vector<HsaMemoryProperties> mem_props(num_mem_banks);
      if (HSAKMT_STATUS_SUCCESS == hsaKmtGetNodeMemoryProperties(
                                       node_id, num_mem_banks, &mem_props[0])) {
        for (uint32_t mem_idx = 0; mem_idx < num_mem_banks; ++mem_idx) {
          // Ignore the one(s) with unknown size.
          if (mem_props[mem_idx].SizeInBytes == 0) {
            continue;
          }

          switch (mem_props[mem_idx].HeapType) {
            case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
            case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
              if (sizeof(void*) == 4) {
                // No gpuvm on 32 bit.
                continue;
              }

            case HSA_HEAPTYPE_GPU_LDS:
            case HSA_HEAPTYPE_GPU_SCRATCH:
              if (gpu != NULL) {
                MemoryRegion* region =
                    new MemoryRegion(false, *gpu, mem_props[mem_idx]);

                gpu->RegisterMemoryProperties(*(region));

                core::Runtime::runtime_singleton_->RegisterMemoryRegion(region);
              }
              break;
            default:
              // TODO: Currently only system, LDS, and GPU memory
              // are valid.
              continue;
          }
        }
      }
    }

    // Create host / system memory region since KFD does not report
    // virtual size.
    const uintptr_t system_base = os::GetUserModeVirtualMemoryBase();
    const size_t system_physical_size = os::GetUsablePhysicalHostMemorySize();
    assert(system_physical_size != 0);

    HsaMemoryProperties default_mem_prop;
    std::memset(&default_mem_prop, 0, sizeof(HsaMemoryProperties));

    default_mem_prop.HeapType = HSA_HEAPTYPE_SYSTEM;
    default_mem_prop.SizeInBytes = (HSAuint64)system_physical_size;
    default_mem_prop.VirtualBaseAddress = (HSAuint64)(system_base);

    MemoryRegion* region = new MemoryRegion(true, *cpu, default_mem_prop);

    if (cpu != NULL) {
      cpu->RegisterMemoryProperties(*region);
    }

    if (gpu != NULL) {
      gpu->RegisterMemoryProperties(*region);
    }

    core::Runtime::runtime_singleton_->RegisterMemoryRegion(region);
  }
}

void Load() {
  // Open KFD
  if (hsaKmtOpenKFD() != HSAKMT_STATUS_SUCCESS) return;

  // Build topology table.
  BuildTopology();

// Load finalizer and extension library
#ifdef HSA_LARGE_MODEL
  std::string extLib[] = {"hsa-runtime-ext64.dll", "libhsa-runtime-ext64.so.1"};
#else
  std::string extLib[] = {"hsa-runtime-ext.dll", "libhsa-runtime-ext.so.1"};
#endif
  core::Runtime::runtime_singleton_->extensions_.Load(
      extLib[os_index(os::current_os)]);
}

// Releases internal resources and unloads DLLs
void Unload() {
  hsaKmtReleaseSystemProperties();

  // Close KFD
  hsaKmtCloseKFD();
}
}  // namespace
