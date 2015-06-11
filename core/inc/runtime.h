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

// HSA runtime C++ interface file.

#ifndef HSA_RUNTME_CORE_INC_RUNTIME_H_
#define HSA_RUNTME_CORE_INC_RUNTIME_H_

#include <vector>
#include <map>

#include "core/inc/hsa_ext_interface.h"
#include "core/inc/hsa_internal.h"

#include "core/inc/agent.h"
#include "core/inc/memory_region.h"
#include "core/inc/memory_database.h"
#include "core/util/utils.h"
#include "core/util/locks.h"
#include "core/util/os.h"

//---------------------------------------------------------------------------//
//    Constants                                                              //
//---------------------------------------------------------------------------//

#define HSA_ARGUMENT_ALIGN_BYTES 16
#define HSA_QUEUE_ALIGN_BYTES 64
#define HSA_PACKET_ALIGN_BYTES 64

namespace core {
extern bool g_use_interrupt_wait;
// class Signal;

/// @brief  Singleton for helper library attach/cleanup.
/// Protects global classes from automatic destruction during process exit.
class Runtime {
 public:
  ExtensionEntryPoints extensions_;

  static Runtime* runtime_singleton_;

  static bool IsOpen();

  static bool Acquire();

  bool Release();

  /// @brief Insert agent into agent list.
  void RegisterAgent(Agent* agent);

  /// @brief Remove agent from agent list.
  void DestroyAgents();

  /// @brief Insert memory region into memory region list.
  void RegisterMemoryRegion(MemoryRegion* region);

  /// @brief Remove memory region from list.
  void DestroyMemoryRegions();

  hsa_status_t GetSystemInfo(hsa_system_info_t attribute, void* value);

  /// @brief Call the user provided call back for each agent in the agent list.
  hsa_status_t IterateAgent(hsa_status_t (*callback)(hsa_agent_t agent,
                                                     void* data),
                            void* data);

  uint32_t GetQueueId();

  /// @brief Memory registration - tracks and provides page aligned regions to
  /// drivers
  bool Register(void* ptr, size_t length, bool registerWithDrivers = true);

  /// @brief Remove memory range from the registration list.
  bool Deregister(void* ptr);

  /// @brief Allocate memory on a particular reigon.
  hsa_status_t AllocateMemory(const MemoryRegion* region, size_t size,
                              void** address);

  /// @brief Free memory previously allocated with AllocateMemory.
  hsa_status_t FreeMemory(void* ptr);

  hsa_status_t AssignMemoryToAgent(void* ptr, const Agent& agent,
                                   hsa_access_permission_t access);

  hsa_status_t CopyMemory(void* dst, const void* src, size_t size);

  /// @brief Backends hookup driver registration APIs in these functions.
  /// The runtime calls this with ranges which are whole pages
  /// and never registers a page more than once.
  bool RegisterWithDrivers(void* ptr, size_t length);
  void DeregisterWithDrivers(void* ptr);

  hsa_status_t SetAsyncSignalHandler(hsa_signal_t signal,
                                     hsa_signal_condition_t cond,
                                     hsa_signal_value_t value,
                                     hsa_amd_signal_handler handler, void* arg);

 private:
  Runtime();

  Runtime(const Runtime&);

  Runtime& operator=(const Runtime&);

  ~Runtime() {}

  void Load();  // for dll attach and KFD open

  void Unload();  // for dll detatch and KFD close

  struct AllocationRegion {
    const MemoryRegion* region;
    const Agent* assigned_agent_;
    size_t size;

    AllocationRegion() : region(NULL), assigned_agent_(NULL), size(0) {}
    AllocationRegion(const MemoryRegion* region_arg, size_t size_arg)
        : region(region_arg), assigned_agent_(NULL), size(size_arg) {}
  };

  const AllocationRegion FindAllocatedRegion(const void* ptr);

  // Will be created before any user could call hsa_init but also could be
  // destroyed before incorrectly written programs call hsa_shutdown.
  static KernelMutex bootstrap_lock_;

  KernelMutex kernel_lock_;

  KernelMutex memory_lock_;

  volatile uint32_t ref_count_;

  // Agent list containing compatible agent in the platform.
  std::vector<Agent*> agents_;

  // Region list containing all physical memory region in the platform.
  std::vector<MemoryRegion*> regions_;

  uint32_t queue_count_;

  uintptr_t system_memory_limit_;

  // Contains list of registered memory.
  MemoryDatabase registered_memory_;

  // Contains the region, address, and size of previously allocated memory.
  std::map<const void*, AllocationRegion> allocation_map_;

  uint64_t sys_clock_freq_;

  struct async_events_control_t {
    hsa_signal_t wake;
    os::Thread async_events_thread_;
    KernelMutex lock;
    bool exit;

    async_events_control_t() : async_events_thread_(NULL) {}
    void Shutdown();
  } async_events_control_;

  struct {
    std::vector<hsa_signal_t> signal_;
    std::vector<hsa_signal_condition_t> cond_;
    std::vector<hsa_signal_value_t> value_;
    std::vector<hsa_amd_signal_handler> handler_;
    std::vector<void*> arg_;

    void push_back(hsa_signal_t signal, hsa_signal_condition_t cond,
                   hsa_signal_value_t value, hsa_amd_signal_handler handler,
                   void* arg) {
      signal_.push_back(signal);
      cond_.push_back(cond);
      value_.push_back(value);
      handler_.push_back(handler);
      arg_.push_back(arg);
    }

    void copy_index(size_t dst, size_t src) {
      signal_[dst] = signal_[src];
      cond_[dst] = cond_[src];
      value_[dst] = value_[src];
      handler_[dst] = handler_[src];
      arg_[dst] = arg_[src];
    }

    size_t size() { return signal_.size(); }

    void pop_back() {
      signal_.pop_back();
      cond_.pop_back();
      value_.pop_back();
      handler_.pop_back();
      arg_.pop_back();
    }

    void clear() {
      signal_.clear();
      cond_.clear();
      value_.clear();
      handler_.clear();
      arg_.clear();
    }

  } async_events_, new_async_events_;
  static void async_events_loop(void*);

  // Frees runtime memory when the runtime library is unloaded if safe to do so.
  // Failure to release the runtime indicates an incorrect application but is
  // common (example: calls library routines at process exit).
  friend class RuntimeCleanup;

  void LoadTools();
  void UnloadTools();
  void CloseTools();
  std::vector<os::LibHandle> tool_libs_;
};

}  // namespace core
#endif  // header guard
