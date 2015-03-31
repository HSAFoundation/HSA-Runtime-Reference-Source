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

#include "inc/hsa.h"
#include "core/inc/hsa_ext_interface.h"

#include "core/inc/agent.h"
#include "core/inc/memory_region.h"
#include "core/inc/memory_database.h"
#include "core/util/utils.h"
#include "core/util/locks.h"

namespace core {
extern bool g_use_interrupt_wait;

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

  bool ExtensionQuery(hsa_extension_t extension);

  hsa_status_t GetSystemInfo(hsa_system_info_t attribute, void* value);

  /// @brief Call the user provided call back for each agent in the agent list.
  hsa_status_t IterateAgent(hsa_status_t (*callback)(hsa_agent_t agent,
                                                     void* data),
                            void* data);

  uint32_t GetQueueId();

  /// @brief Memory registration - tracks and provides page aligned regions to
  /// drivers
  bool Register(void* ptr, size_t length);

  /// @brief Remove memory range from the registration list.
  void Deregister(void* ptr);

  /// @brief Allocate memory on a particular reigon.
  hsa_status_t AllocateMemory(const MemoryRegion* region, size_t size,
                              void** address);

  /// @brief Free memory previously allocated with AllocateMemory.
  hsa_status_t FreeMemory(void* ptr);

  /// @brief Backends hookup driver registration APIs in these functions.
  /// The runtime calls this with ranges which are whole pages
  /// and never registers a page more than once.
  bool RegisterWithDrivers(void* ptr, size_t length);
  void DeregisterWithDrivers(void* ptr);

 private:
  Runtime() : ref_count_(0), queue_count_(0) {}

  Runtime(const Runtime&);

  Runtime& operator=(const Runtime&);

  ~Runtime() {}

  void Load();  // for dll attach and KFD open

  void Unload();  // for dll detatch and KFD close

  struct AllocationRegion {
    const MemoryRegion* region;
    size_t size;

    AllocationRegion() {}
    AllocationRegion(const MemoryRegion* region_arg, size_t size_arg)
        : region(region_arg), size(size_arg) {}
  };

  // Will be created before any user could call hsa_init but also could be
  // destroyed before incorrectly written programs call hsa_shutdown.
  static KernelMutex bootstrap_lock_;

  KernelMutex kernel_lock_;

  volatile uint32_t ref_count_;

  // Agent list containing compatible agent in the platform.
  std::vector<Agent*> agents_;

  uint32_t queue_count_;

  // Contains list of registered memory.
  MemoryDatabase registered_memory_;

  // Contains the region, address, and size of previously allocated memory.
  std::map<void*, AllocationRegion> allocation_map_;

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
