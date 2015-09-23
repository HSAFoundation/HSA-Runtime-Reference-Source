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
#include <cstring>
#include <string>
#include <vector>

#include "core/common/shared.h"

#include "core/inc/hsa_ext_interface.h"
#include "core/inc/amd_memory_region.h"
#include "core/inc/amd_memory_registration.h"
#include "core/inc/amd_topology.h"
#include "core/inc/signal.h"
#include "core/inc/thunk.h"

#include "core/inc/hsa_api_trace_int.h"

#define HSA_VERSION_MAJOR 1
#define HSA_VERSION_MINOR 0

namespace core {
bool g_use_interrupt_wait = true;

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

void Runtime::RegisterMemoryRegion(MemoryRegion* region) {
  regions_.push_back(region);

  if (reinterpret_cast<amd::MemoryRegion*>(region)->IsSystem()) {
    assert(system_region_.handle == 0);
    system_region_ = MemoryRegion::Convert(region);
}
}

void Runtime::DestroyMemoryRegions() {
  std::for_each(regions_.begin(), regions_.end(), DeleteObject());
  regions_.clear();
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
      assert(sys_clock_freq_ != 0 &&
             "Use of HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY before HSA "
             "initialization completes.");
      *(uint64_t*)value = sys_clock_freq_;
      break;
    }
    case HSA_SYSTEM_INFO_SIGNAL_MAX_WAIT:
      *((uint64_t*)value) = 0xFFFFFFFFFFFFFFFF;
      break;
    case HSA_SYSTEM_INFO_ENDIANNESS:
#if defined(HSA_LITTLE_ENDIAN)
      *((hsa_endianness_t*)value) = HSA_ENDIANNESS_LITTLE;
#else
      *((hsa_endianness_t*)value) = HSA_ENDIANNESS_BIG;
#endif
      break;
    case HSA_SYSTEM_INFO_MACHINE_MODEL:
#if defined(HSA_LARGE_MODEL)
      *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_LARGE;
#else
      *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_SMALL;
#endif
      break;
    case HSA_SYSTEM_INFO_EXTENSIONS:
      memset(value, 0, sizeof(uint8_t) * 128);

      if (extensions_.table.hsa_ext_program_finalize_fn != NULL) {
        *((uint8_t*)value) = 1 << HSA_EXTENSION_FINALIZER;
      }

      if (extensions_.table.hsa_ext_image_create_fn != NULL) {
        *((uint8_t*)value) |= 1 << HSA_EXTENSION_IMAGES;
      }

      *((uint8_t*)value) |= 1 << HSA_EXTENSION_AMD_PROFILER;

      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Runtime::IterateAgent(hsa_status_t (*callback)(hsa_agent_t agent,
                                                            void* data),
                                   void* data) {
  const size_t num_agent = agents_.size();

  if (!IsOpen()) {
    return HSA_STATUS_ERROR_NOT_INITIALIZED;
  }

  for (size_t i = 0; i < num_agent; ++i) {
    hsa_agent_t agent = Agent::Convert(agents_[i]);
    hsa_status_t status = callback(agent, data);

    if (status != HSA_STATUS_SUCCESS) {
      return status;
    }
  }

  return HSA_STATUS_SUCCESS;
}

uint32_t Runtime::GetQueueId() { return atomic::Increment(&queue_count_); }

amd::LoaderContext* Runtime::loader_context() { return &loader_context_; }
amd::hsa::code::AmdHsaCodeManager* Runtime::code_manager() { return &code_manager_; }

bool Runtime::Register(void* ptr, size_t length, bool registerWithDrivers) {
  return registered_memory_.Register(ptr, length, registerWithDrivers);
}

bool Runtime::Deregister(void* ptr) {
  return registered_memory_.Deregister(ptr);
}

hsa_status_t Runtime::AllocateMemory(const MemoryRegion* region, size_t size,
                                     void** ptr) {
  bool allocation_allowed = false;
  region->GetInfo(HSA_REGION_INFO_RUNTIME_ALLOC_ALLOWED, &allocation_allowed);
  if (!allocation_allowed) {
    return HSA_STATUS_ERROR_INVALID_ALLOCATION;
  }

  size_t allocation_max = 0;
  region->GetInfo(HSA_REGION_INFO_ALLOC_MAX_SIZE, &allocation_max);
  if (size > allocation_max) {
    return HSA_STATUS_ERROR_INVALID_ALLOCATION;
  }

  size_t allocation_granule = 0;
  region->GetInfo(HSA_REGION_INFO_RUNTIME_ALLOC_GRANULE, &allocation_granule);
  assert(IsPowerOfTwo(allocation_granule));

  size = AlignUp(size, allocation_granule);
  hsa_status_t status = region->Allocate(size, ptr);

  // Track the allocation result so that it could be freed properly.
  if (status == HSA_STATUS_SUCCESS) {
    assert(*ptr != NULL);
    ScopedAcquire<KernelMutex> lock(&memory_lock_);
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
    ScopedAcquire<KernelMutex> lock(&memory_lock_);

    std::map<const void*, AllocationRegion>::const_iterator it =
        allocation_map_.find(ptr);

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

hsa_status_t Runtime::AssignMemoryToAgent(void* ptr, const Agent& agent,
                                          hsa_access_permission_t access) {
  const uintptr_t uptr = reinterpret_cast<uintptr_t>(ptr);
  if (uptr < system_memory_limit_) {
    // System memory is always fine grain.
    return HSA_STATUS_SUCCESS;
  }

  Runtime::AllocationRegion allocation_region;
  {
    ScopedAcquire<KernelMutex> lock(&memory_lock_);
    std::map<const void*, AllocationRegion>::const_iterator it =
        allocation_map_.find(ptr);

    if (it == allocation_map_.end()) {
      return HSA_STATUS_ERROR;
    }

    allocation_region = it->second;

    assert(allocation_region.region != NULL);
    assert(allocation_region.size != 0);
  }

  if (allocation_region.assigned_agent_ == &agent) {
    // Already assigned to the selected agent.
    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t status = allocation_region.region->AssignAgent(
      ptr, allocation_region.size, agent, access);

  if (status == HSA_STATUS_SUCCESS) {
    ScopedAcquire<KernelMutex> lock(&memory_lock_);
    allocation_map_[ptr].assigned_agent_ = &agent;
  }

  return status;
}

hsa_status_t Runtime::CopyMemory(void* dst, const void* src, size_t size) {
  const uintptr_t dst_uptr = reinterpret_cast<uintptr_t>(dst);
  const uintptr_t src_uptr = reinterpret_cast<uintptr_t>(src);

  const bool is_dst_system = (dst_uptr < system_memory_limit_);
  const bool is_src_system = (src_uptr < system_memory_limit_);

  if (is_dst_system && is_src_system) {
    // Both source and destination are system memory.
    memmove(dst, src, size);
    return HSA_STATUS_SUCCESS;
  }

  const Agent* dst_agent = NULL;
  const Agent* src_agent = NULL;

  if (!is_dst_system) {
    dst_agent = FindAllocatedRegion(dst).assigned_agent_;
    assert(dst_agent != NULL &&
           dst_agent->device_type() == Agent::kAmdGpuDevice);
  }

  if (!is_src_system) {
    src_agent = FindAllocatedRegion(src).assigned_agent_;
    assert(src_agent != NULL &&
           src_agent->device_type() == Agent::kAmdGpuDevice);
  }

  if (dst_agent != NULL && src_agent != NULL && dst_agent != src_agent) {
    // TODO: not implemented yet until we could clarify GPU-GPU transfer.
    return HSA_STATUS_ERROR;
  }

  const Agent* agent = (dst_agent != NULL) ? dst_agent : src_agent;

  if (agent == NULL) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  return const_cast<Agent*>(agent)->DmaCopy(dst, src, size);
}

bool Runtime::RegisterWithDrivers(void* ptr, size_t length) {
  return amd::RegisterKfdMemory(ptr, length);
}

void Runtime::DeregisterWithDrivers(void* ptr) {
  amd::DeregisterKfdMemory(ptr);
}

Runtime::Runtime() : ref_count_(0), queue_count_(0), sys_clock_freq_(0) {
  system_memory_limit_ =
      os::GetUserModeVirtualMemoryBase() + os::GetUserModeVirtualMemorySize();
  system_region_.handle = 0;
}

void Runtime::Load() {
  // Load interrupt enable option
  std::string interrupt = os::GetEnvVar("HSA_ENABLE_INTERRUPT");
  g_use_interrupt_wait = (interrupt != "0");

  amd::Load();

  // Setup system region allocator.
  if (reinterpret_cast<amd::MemoryRegion*>(
          core::MemoryRegion::Convert(system_region_))->fine_grain()) {
    system_allocator_ = [](size_t size, size_t alignment) -> void * {
      return _aligned_malloc(size, alignment);
    };

    system_deallocator_ = [](void* ptr) { _aligned_free(ptr); };
  } else {
    // TODO(bwicakso): might need memory pooling to cover allocation that
    // requires less than 4096 bytes.
    system_allocator_ = [&](size_t size, size_t alignment) -> void * {
      assert(alignment <= 4096);
      void* ptr = NULL;
      return (HSA_STATUS_SUCCESS ==
              HSA::hsa_memory_allocate(system_region_, size, &ptr))
                 ? ptr
                 : NULL;
    };

    system_deallocator_ = [](void* ptr) { HSA::hsa_memory_free(ptr); };
  }

  BaseShared::SetAllocateAndFree(system_allocator_, system_deallocator_);

  // Cache system clock frequency
  HsaClockCounters clocks;
  hsaKmtGetClockCounters(0, &clocks);
  sys_clock_freq_ = clocks.SystemClockFrequencyHz;

  // Load extensions
  LoadExtensions();

  // Load tools libraries
  LoadTools();
}

void Runtime::Unload() {
  UnloadTools();
  UnloadExtensions();
  loader_context_.Reset();
  DestroyAgents();
  DestroyMemoryRegions();
  CloseTools();

  async_events_control_.Shutdown();

  amd::Unload();

  system_region_.handle = 0;
}

void Runtime::LoadExtensions() {
// Load finalizer and extension library
#ifdef HSA_LARGE_MODEL
  static const std::string kFinalizerLib[] = {"hsa-ext-finalize64.dll",
                                              "libhsa-ext-finalize64.so.1"};
  static const std::string kImageLib[] = {"hsa-ext-image64.dll",
                                          "libhsa-ext-image64.so.1"};
#else
  static const std::string kFinalizerLib[] = {"hsa-ext-finalize.dll",
                                              "libhsa-ext-finalize.so.1"};
  static const std::string kImageLib[] = {"hsa-ext-image.dll",
                                          "libhsa-ext-image.so.1"};
#endif
  extensions_.Load(kFinalizerLib[os_index(os::current_os)]);
  extensions_.Load(kImageLib[os_index(os::current_os)]);
}

void Runtime::UnloadExtensions() {
  extensions_.Unload();
}

void Runtime::LoadTools() {
  typedef bool (*tool_init_t)(::ApiTable*, uint64_t, uint64_t,
                              const char* const*);
  typedef Agent* (*tool_wrap_t)(Agent*);
  typedef void (*tool_add_t)(Runtime*);

  // Link extensions to API interception
  hsa_api_table_.LinkExts(&extensions_.table);

  // Load tool libs
  std::string tool_names = os::GetEnvVar("HSA_TOOLS_LIB");
  if (tool_names != "") {
    std::vector<std::string> names = parse_tool_names(tool_names);
    std::vector<const char*> failed;
    for (int i = 0; i < names.size(); i++) {
      os::LibHandle tool = os::LoadLib(names[i]);

      if (tool != NULL) {
        tool_libs_.push_back(tool);

        size_t size = agents_.size();

        tool_init_t ld;
        ld = (tool_init_t)os::GetExportAddress(tool, "OnLoad");
        if (ld) {
          if (!ld(&hsa_api_table_.table, 0,
                  failed.size(), &failed[0])) {
            failed.push_back(names[i].c_str());
            os::CloseLib(tool);
            continue;
          }
        }
        tool_wrap_t wrap;
        wrap = (tool_wrap_t)os::GetExportAddress(tool, "WrapAgent");
        if (wrap) {
          for (int j = 0; j < size; j++) {
            Agent* agent = wrap(agents_[j]);
            if (agent != NULL) {
              assert(agent->IsValid() &&
                     "Agent returned from WrapAgent is not valid");
              agents_[j] = agent;
            }
          }
        }

        tool_add_t add;
        add = (tool_add_t)os::GetExportAddress(tool, "AddAgent");
        if (add) add(this);
      }
    }
  }
}

void Runtime::UnloadTools() {
  typedef void (*tool_unload_t)();
  for (size_t i = tool_libs_.size(); i != 0; i--) {
    tool_unload_t unld;
    unld = (tool_unload_t)os::GetExportAddress(tool_libs_[i - 1], "OnUnload");
    if (unld) unld();
  }

  // Reset API table in case some tool doesn't cleanup properly
  hsa_api_table_.Reset();
}

void Runtime::CloseTools() {
  for (int i = 0; i < tool_libs_.size(); i++) os::CloseLib(tool_libs_[i]);
  tool_libs_.clear();
}

const Runtime::AllocationRegion Runtime::FindAllocatedRegion(const void* ptr) {
  ScopedAcquire<KernelMutex> lock(&memory_lock_);

  const uintptr_t uptr = reinterpret_cast<uintptr_t>(ptr);
  Runtime::AllocationRegion invalid_region;

  if (allocation_map_.empty() || uptr < system_memory_limit_) {
    return invalid_region;
  }

  // Find the last element in the allocation list that has address
  // less or equal to ptr.
  std::map<const void*, AllocationRegion>::const_iterator it =
      allocation_map_.upper_bound(ptr);

  if (it == allocation_map_.begin()) {
    // All elements have address larger than ptr.
    return invalid_region;
  }

  --it;

  const uintptr_t start_address = reinterpret_cast<uintptr_t>(it->first);
  const uintptr_t end_address = start_address + it->second.size;

  if (uptr >= start_address && uptr < end_address) {
    return it->second;
  }

  return invalid_region;
}

void Runtime::async_events_control_t::Shutdown() {
  if (async_events_thread_ != NULL) {
    exit = true;
    hsa_signal_handle(wake)->StoreRelaxed(1);
    os::WaitForThread(async_events_thread_);
    os::CloseThread(async_events_thread_);
    async_events_thread_ = NULL;
    HSA::hsa_signal_destroy(wake);
  }
}

hsa_status_t Runtime::SetAsyncSignalHandler(hsa_signal_t signal,
                                            hsa_signal_condition_t cond,
                                            hsa_signal_value_t value,
                                            hsa_amd_signal_handler handler,
                                            void* arg) {
  // Asyncronous signal handler is only supported when KFD events are on.
  if (!core::g_use_interrupt_wait) return HSA_STATUS_ERROR_OUT_OF_RESOURCES;

  // Indicate that this signal is in use.
  hsa_signal_handle(signal)->Retain();

  ScopedAcquire<KernelMutex>(&async_events_control_.lock);

  // Lazy initializer
  if (async_events_control_.async_events_thread_ == NULL) {
    // Create monitoring thread control signal
    auto err = HSA::hsa_signal_create(0, 0, NULL, &async_events_control_.wake);
    if (err != HSA_STATUS_SUCCESS) {
      assert(false && "Asyncronous events control signal creation error.");
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    }
    async_events_.push_back(async_events_control_.wake, HSA_SIGNAL_CONDITION_NE,
                            0, NULL, NULL);

    // Start event monitoring thread
    async_events_control_.exit = false;
    async_events_control_.async_events_thread_ =
        os::CreateThread(async_events_loop, NULL);
    if (async_events_control_.async_events_thread_ == NULL) {
      assert(false && "Asyncronous events thread creation error.");
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    }
  }

  new_async_events_.push_back(signal, cond, value, handler, arg);

  hsa_signal_handle(async_events_control_.wake)->StoreRelease(1);

  return HSA_STATUS_SUCCESS;
}

void Runtime::async_events_loop(void*) {
  auto& async_events_control_ = runtime_singleton_->async_events_control_;
  auto& async_events_ = runtime_singleton_->async_events_;
  auto& new_async_events_ = runtime_singleton_->new_async_events_;

  while (!async_events_control_.exit) {
    // Wait for a signal
    hsa_signal_value_t value;
    uint32_t index = hsa_amd_signal_wait_any(
        uint32_t(async_events_.size()), &async_events_.signal_[0],
        &async_events_.cond_[0], &async_events_.value_[0], uint64_t(-1),
        HSA_WAIT_STATE_BLOCKED, &value);

    // Reset the control signal
    if (index == 0) {
      hsa_signal_handle(async_events_control_.wake)->StoreRelaxed(0);
    } else if (index != -1) {
      // No error or timout occured, process the handler
      bool keep =
          async_events_.handler_[index](value, async_events_.arg_[index]);
      if (!keep) {
        hsa_signal_handle(async_events_.signal_[index])->Release();
        async_events_.copy_index(index, async_events_.size() - 1);
        async_events_.pop_back();
      }
    }

    // Insert new signals
    {
      ScopedAcquire<KernelMutex>(&async_events_control_.lock);
      for (size_t i = 0; i < new_async_events_.size(); i++)
        async_events_.push_back(
            new_async_events_.signal_[i], new_async_events_.cond_[i],
            new_async_events_.value_[i], new_async_events_.handler_[i],
            new_async_events_.arg_[i]);
      new_async_events_.clear();
    }

    // Check for dead signals
    index = 0;
    while (index != async_events_.size()) {
      if (!hsa_signal_handle(async_events_.signal_[index])->IsValid()) {
        hsa_signal_handle(async_events_.signal_[index])->Release();
        async_events_.copy_index(index, async_events_.size() - 1);
        async_events_.pop_back();
        continue;
      }
      index++;
    }
  }

  // Release wait count of all pending signals
  for (size_t i = 1; i < async_events_.size(); i++)
    hsa_signal_handle(async_events_.signal_[i])->Release();
  async_events_.clear();

  for (size_t i = 0; i < new_async_events_.size(); i++)
    hsa_signal_handle(new_async_events_.signal_[i])->Release();
  new_async_events_.clear();
}

}  // namespace core
