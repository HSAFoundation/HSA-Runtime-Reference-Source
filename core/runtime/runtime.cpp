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

#include "core/inc/runtime.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/inc/hsa_ext_interface.h"
#include "core/inc/amd_memory_registration.h"
#include "core/inc/amd_topology.h"
#include "core/inc/thunk.h"

#include "inc/hsa_api_trace.h"

#define HSA_VERSION_MAJOR 0
#define HSA_VERSION_MINOR 187

namespace core {
bool g_use_interrupt_wait = false;

Runtime* Runtime::runtime_singleton_ = NULL;

KernelMutex Runtime::bootstrap_lock_;

static bool loaded = true;

class RuntimeCleanup {
 public:
  ~RuntimeCleanup() {
    if (!Runtime::IsOpen()) delete Runtime::runtime_singleton_;
    loaded = false;
  }
};
static RuntimeCleanup cleanup_at_unload_;

static std::vector<std::string> parse_tool_names(std::string tool_names) {
  std::vector<std::string> names;
  std::string name = "";
  bool quoted = false;
  while (tool_names.size() != 0) {
    auto index = tool_names.find_first_of(" \"\\");
    if (index == std::string::npos) {
      name += tool_names;
      break;
    }
    switch (tool_names[index]) {
      case ' ': {
        if (!quoted) {
          name += tool_names.substr(0, index);
          tool_names.erase(0, index + 1);
          names.push_back(name);
          name = "";
        } else {
          name += tool_names.substr(0, index + 1);
          tool_names.erase(0, index + 1);
        }
        break;
      }
      case '\"': {
        if (quoted) {
          quoted = false;
          name += tool_names.substr(0, index);
          tool_names.erase(0, index + 1);
          names.push_back(name);
          name = "";
        } else {
          quoted = true;
          tool_names.erase(0, index + 1);
        }
        break;
      }
      case '\\': {
        if (tool_names.size() > index + 1) {
          name += tool_names.substr(0, index) + tool_names[index + 1];
          tool_names.erase(0, index + 2);
        }
        break;
      }
    }  // end switch
  }    // end while

  if (name != "") names.push_back(name);
  return names;
}

bool Runtime::IsOpen() {
  return (Runtime::runtime_singleton_ != NULL) &&
         (Runtime::runtime_singleton_->ref_count_ != 0);
}

bool Runtime::Acquire() {
  // Check to see if HSA has been cleaned up (process exit)
  if (!loaded) return false;

  // Handle initialization races
  ScopedAcquire<KernelMutex> boot(&bootstrap_lock_);
  if (runtime_singleton_ == NULL) runtime_singleton_ = new Runtime();

  // Serialize with release
  ScopedAcquire<KernelMutex> lock(&runtime_singleton_->kernel_lock_);
  if (runtime_singleton_->ref_count_ == INT32_MAX) return false;
  runtime_singleton_->ref_count_++;
  if (runtime_singleton_->ref_count_ == 1) runtime_singleton_->Load();
  return true;
}

bool Runtime::Release() {
  ScopedAcquire<KernelMutex> lock(&kernel_lock_);
  if (ref_count_ == 0) return false;
  if (ref_count_ == 1)  // Release all registered memory, then unload backends
  {
    registered_memory_.DeregisterAll();
    Unload();
  }
  ref_count_--;
  return true;
}

void Runtime::RegisterAgent(Agent* agent) { agents_.push_back(agent); }

void Runtime::DestroyAgents() {
  std::for_each(agents_.begin(), agents_.end(), DeleteObject());
  agents_.clear();
}

bool Runtime::ExtensionQuery(hsa_extension_t extension) {
  switch (extension) {
    case HSA_EXT_FINALIZER: {
      return extensions_.hsa_ext_finalize_program != NULL;
      break;
    }
    case HSA_EXT_LINKER: {
      return extensions_.hsa_ext_finalize_program != NULL;
      break;
    }
    case HSA_EXT_IMAGES: {
      return extensions_.hsa_ext_image_clear != NULL;
      break;
    }
    case HSA_EXT_AMD_PROFILER: {
      return true;
      break;
    }
    default:
      return false;
  }
}

hsa_status_t Runtime::GetSystemInfo(hsa_system_info_t attribute, void* value) {
  switch (attribute) {
    case HSA_SYSTEM_INFO_VERSION_MAJOR:
      *((uint16_t*)value) = HSA_VERSION_MAJOR;
      break;
    case HSA_SYSTEM_INFO_VERSION_MINOR:
      *((uint16_t*)value) = HSA_VERSION_MINOR;
      break;
    case HSA_SYSTEM_INFO_TIMESTAMP: {
      HsaClockCounters clocks;
      hsaKmtGetClockCounters(0, &clocks);
      *((uint64_t*)value) = clocks.SystemClockCounter;
      break;
    }
    case HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY: {
      HsaClockCounters clocks;
      hsaKmtGetClockCounters(0, &clocks);
      *(uint64_t*)value = clocks.SystemClockFrequencyHz;
      break;
    }
    case HSA_SYSTEM_INFO_SIGNAL_MAX_WAIT:
      *((uint64_t*)value) = 0xFFFFFFFFFFFFFFFF;
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Runtime::IterateAgent(hsa_status_t (*callback)(hsa_agent_t agent,
                                                            void* data),
                                   void* data) {
  const size_t num_component = agents_.size();

  if (!IsOpen()) {
    return HSA_STATUS_ERROR_NOT_INITIALIZED;
  }

  for (size_t i = 0; i < num_component; ++i) {
    hsa_agent_t agent = Agent::Convert(agents_[i]);
    hsa_status_t status = callback(agent, data);

    if (status != HSA_STATUS_SUCCESS) {
      return status;
    }
  }

  return HSA_STATUS_SUCCESS;
}

uint32_t Runtime::GetQueueId() { return atomic::Increment(&queue_count_); }

bool Runtime::Register(void* ptr, size_t length) {
  return registered_memory_.Register(ptr, length);
}

void Runtime::Deregister(void* ptr) { registered_memory_.Deregister(ptr); }

hsa_status_t Runtime::AllocateMemory(const MemoryRegion* region, size_t size,
                                     void** ptr) {
  size_t allocation_max = 0;
  region->GetInfo(HSA_REGION_INFO_ALLOC_MAX_SIZE, &allocation_max);
  if (size > allocation_max) {
    return HSA_STATUS_ERROR_INVALID_ALLOCATION;
  }

  size_t allocation_granule = 0;
  region->GetInfo(HSA_REGION_INFO_ALLOC_GRANULE, &allocation_granule);
  assert(IsPowerOfTwo(allocation_granule));

  hsa_status_t status =
      region->Allocate(AlignUp(size, allocation_granule), ptr);

  // Track the allocation result so that it could be freed properly.
  if (status == HSA_STATUS_SUCCESS) {
    assert(*ptr != NULL);
    ScopedAcquire<KernelMutex> lock(&kernel_lock_);
    allocation_map_[*ptr] = AllocationRegion(region, size);
  }

  return status;
}

hsa_status_t Runtime::FreeMemory(void* ptr) {
  if (ptr == NULL) {
    return HSA_STATUS_SUCCESS;
  }

  const MemoryRegion* region = NULL;
  size_t size = 0;
  {
    ScopedAcquire<KernelMutex> lock(&kernel_lock_);

    std::map<void*, AllocationRegion>::iterator it = allocation_map_.find(ptr);

    if (it == allocation_map_.end()) {
      assert(false && "Can't find address in allocation map");
      return HSA_STATUS_ERROR;
    }

    region = it->second.region;
    size = it->second.size;

    allocation_map_.erase(it);
  }

  return region->Free(ptr, size);
}

bool Runtime::RegisterWithDrivers(void* ptr, size_t length) {
  return amd::RegisterKfdMemory(ptr, length);
}

void Runtime::DeregisterWithDrivers(void* ptr) {
  amd::DeregisterKfdMemory(ptr);
}

void Runtime::Load() {
  // Load interrupt enable option
  std::string interrupt = os::GetEnvVar("HSA_ENABLE_INTERRUPT");
  g_use_interrupt_wait = (interrupt == "1");

  amd::Load();

  // Load tools libraries
  LoadTools();
}

void Runtime::Unload() {
  UnloadTools();
  DestroyAgents();
  CloseTools();
  extensions_.Unload();
  amd::Unload();
}

void Runtime::LoadTools() {
#ifndef HSA_NO_TOOLS_EXTENSION
  typedef void (*tool_init_t)(ApiTable*);
  typedef Agent* (*tool_wrap_t)(Agent*);
  typedef Agent* (*tool_add_t)(Runtime*);

  std::string tool_names = os::GetEnvVar("HSA_TOOLS_LIB");
  if (tool_names != "") {
    std::vector<std::string> names = parse_tool_names(tool_names);
    for (int i = 0; i < names.size(); i++) {
      os::LibHandle tool = os::LoadLib(names[i]);

      if (tool != NULL) {
        tool_libs_.push_back(tool);

        size_t size = agents_.size();

        tool_init_t ld;
        ld = (tool_init_t)os::GetExportAddress(tool, "Init");
        if (ld) ld(&hsa_api_table_);

        tool_wrap_t wrap;
        wrap = (tool_wrap_t)os::GetExportAddress(tool, "WrapAgent");
        if (wrap) {
          for (int j = 0; j < size; j++) agents_[j] = wrap(agents_[j]);
        }

        tool_add_t add;
        add = (tool_add_t)os::GetExportAddress(tool, "AddAgent");
        if (add) add(this);
      }
    }
  }
#endif
}

void Runtime::UnloadTools() {
#ifndef HSA_NO_TOOLS_EXTENSION
  typedef void (*tool_unload_t)();
  for (size_t i = tool_libs_.size(); i != 0; i--) {
    tool_unload_t unld;
    unld = (tool_unload_t)os::GetExportAddress(tool_libs_[i - 1], "Unload");
    if (unld) unld();
  }

  // Reset API table in case some tool doesn't cleanup properly
  new (&hsa_api_table_) ApiTable();
#endif
}

void Runtime::CloseTools()
{
  for (int i = 0; i < tool_libs_.size(); i++) os::CloseLib(tool_libs_[i]);
  tool_libs_.clear();
}

}  // namespace core
