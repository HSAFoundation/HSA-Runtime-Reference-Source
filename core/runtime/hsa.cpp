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

// HSA C to C++ interface implementation.
// This file does argument checking and conversion to C++.
#include <cstring>
#include <set>

#include "core/inc/runtime.h"
#include "core/inc/agent.h"
#include "core/inc/host_queue.h"
#include "core/inc/memory_region.h"
#include "core/inc/queue.h"
#include "core/inc/signal.h"
#include "core/inc/default_signal.h"
#include "core/inc/interrupt_signal.h"
#include "core/loader/executable.hpp"
#include "core/loader/isa.hpp"

template <class T>
struct ValidityError;
template <>
struct ValidityError<core::Signal*> {
  enum { kValue = HSA_STATUS_ERROR_INVALID_SIGNAL };
};
template <>
struct ValidityError<core::Agent*> {
  enum { kValue = HSA_STATUS_ERROR_INVALID_AGENT };
};
template <>
struct ValidityError<core::MemoryRegion*> {
  enum { kValue = HSA_STATUS_ERROR_INVALID_REGION };
};
template <>
struct ValidityError<core::Queue*> {
  enum { kValue = HSA_STATUS_ERROR_INVALID_QUEUE };
};
template <>
struct ValidityError<core::loader::Isa*> {
  enum { kValue = HSA_STATUS_ERROR_INVALID_ISA };
};
template <class T>
struct ValidityError<const T*> {
  enum { kValue = ValidityError<T*>::kValue };
};

#define IS_BAD_PTR(ptr)                                          \
  do {                                                           \
    if ((ptr) == NULL) return HSA_STATUS_ERROR_INVALID_ARGUMENT; \
  } while (false)
#define IS_VALID(ptr)                                            \
  do {                                                           \
    if (((ptr) == NULL) || !((ptr)->IsValid()))                  \
      return hsa_status_t(ValidityError<decltype(ptr)>::kValue); \
  } while (false)
#define CHECK_ALLOC(ptr)                                         \
  do {                                                           \
    if ((ptr) == NULL) return HSA_STATUS_ERROR_OUT_OF_RESOURCES; \
  } while (false)
#define IS_OPEN()                                     \
  do {                                                \
    if (!core::Runtime::runtime_singleton_->IsOpen()) \
      return HSA_STATUS_ERROR_NOT_INITIALIZED;        \
  } while (false)

template <class T>
static __forceinline bool IsValid(T* ptr) {
  return (ptr == NULL) ? NULL : ptr->IsValid();
}

//-----------------------------------------------------------------------------
// Basic Checks
//-----------------------------------------------------------------------------
static_assert(sizeof(hsa_barrier_and_packet_t) ==
                  sizeof(hsa_kernel_dispatch_packet_t),
              "AQL packet definitions have wrong sizes!");
static_assert(sizeof(hsa_barrier_and_packet_t) ==
                  sizeof(hsa_agent_dispatch_packet_t),
              "AQL packet definitions have wrong sizes!");
static_assert(sizeof(hsa_barrier_and_packet_t) == 64,
              "AQL packet definitions have wrong sizes!");
static_assert(sizeof(hsa_barrier_and_packet_t) ==
                  sizeof(hsa_barrier_or_packet_t),
              "AQL packet definitions have wrong sizes!");
#ifdef HSA_LARGE_MODEL
static_assert(sizeof(void*) == 8, "HSA_LARGE_MODEL is set incorrectly!");
#else
static_assert(sizeof(void*) == 4, "HSA_LARGE_MODEL is set incorrectly!");
#endif

namespace HSA {

//---------------------------------------------------------------------------//
//  Init/Shutdown routines
//---------------------------------------------------------------------------//
hsa_status_t HSA_API hsa_init() {
  if (core::Runtime::runtime_singleton_->Acquire()) return HSA_STATUS_SUCCESS;
  return HSA_STATUS_ERROR_REFCOUNT_OVERFLOW;
}

hsa_status_t HSA_API hsa_shut_down() {
  if (core::Runtime::runtime_singleton_->Release()) return HSA_STATUS_SUCCESS;
  return HSA_STATUS_ERROR_NOT_INITIALIZED;
}

//---------------------------------------------------------------------------//
//  System
//---------------------------------------------------------------------------//
hsa_status_t HSA_API
    hsa_system_get_info(hsa_system_info_t attribute, void* value) {
  IS_OPEN();
  return core::Runtime::runtime_singleton_->GetSystemInfo(attribute, value);
}

hsa_status_t HSA_API
    hsa_system_extension_supported(uint16_t extension, uint16_t version_major,
                                   uint16_t version_minor, bool* result) {
  IS_OPEN();

  if ((extension > HSA_EXTENSION_AMD_PROFILER) || (result == NULL)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  *result = false;

  uint16_t system_version_major = 0;
  hsa_status_t status = core::Runtime::runtime_singleton_->GetSystemInfo(
      HSA_SYSTEM_INFO_VERSION_MAJOR, &system_version_major);
  assert(status == HSA_STATUS_SUCCESS);

  if (version_major <= system_version_major) {
    uint16_t system_version_minor = 0;
    status = core::Runtime::runtime_singleton_->GetSystemInfo(
        HSA_SYSTEM_INFO_VERSION_MINOR, &system_version_minor);
    assert(status == HSA_STATUS_SUCCESS);

    if (version_minor <= system_version_minor) {
      *result = true;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
    hsa_system_get_extension_table(uint16_t extension, uint16_t version_major,
                                   uint16_t version_minor, void* table) {
  if (table == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  IS_OPEN();

  bool supported = false;
  hsa_status_t status = hsa_system_extension_supported(
      extension, version_major, version_minor, &supported);

  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  if (supported) {
    ExtTable& runtime_ext_table =
        core::Runtime::runtime_singleton_->extensions_.table;

    if (extension == HSA_EXTENSION_IMAGES) {
      // Currently there is only version 1.00.
      hsa_ext_images_1_00_pfn_t* ext_table =
          reinterpret_cast<hsa_ext_images_1_00_pfn_t*>(table);
      ext_table->hsa_ext_image_clear = runtime_ext_table.hsa_ext_image_clear;
      ext_table->hsa_ext_image_copy = runtime_ext_table.hsa_ext_image_copy;
      ext_table->hsa_ext_image_create = runtime_ext_table.hsa_ext_image_create;
      ext_table->hsa_ext_image_data_get_info =
          runtime_ext_table.hsa_ext_image_data_get_info;
      ext_table->hsa_ext_image_destroy =
          runtime_ext_table.hsa_ext_image_destroy;
      ext_table->hsa_ext_image_export = runtime_ext_table.hsa_ext_image_export;
      ext_table->hsa_ext_image_get_capability =
          runtime_ext_table.hsa_ext_image_get_capability;
      ext_table->hsa_ext_image_import = runtime_ext_table.hsa_ext_image_import;
      ext_table->hsa_ext_sampler_create =
          runtime_ext_table.hsa_ext_sampler_create;
      ext_table->hsa_ext_sampler_destroy =
          runtime_ext_table.hsa_ext_sampler_destroy;

      return HSA_STATUS_SUCCESS;
    } else if (extension == HSA_EXTENSION_FINALIZER) {
      // Currently there is only version 1.00.
      hsa_ext_finalizer_1_00_pfn_s* ext_table =
          reinterpret_cast<hsa_ext_finalizer_1_00_pfn_s*>(table);
      ext_table->hsa_ext_program_add_module =
          runtime_ext_table.hsa_ext_program_add_module;
      ext_table->hsa_ext_program_create =
          runtime_ext_table.hsa_ext_program_create;
      ext_table->hsa_ext_program_destroy =
          runtime_ext_table.hsa_ext_program_destroy;
      ext_table->hsa_ext_program_finalize =
          runtime_ext_table.hsa_ext_program_finalize;
      ext_table->hsa_ext_program_get_info =
          runtime_ext_table.hsa_ext_program_get_info;
      ext_table->hsa_ext_program_iterate_modules =
          runtime_ext_table.hsa_ext_program_iterate_modules;

      return HSA_STATUS_SUCCESS;
    } else {
      // TODO: other extensions are not yet implemented.
      return HSA_STATUS_ERROR;
    }
  }

  return HSA_STATUS_SUCCESS;
}

//---------------------------------------------------------------------------//
//  Agent
//---------------------------------------------------------------------------//
hsa_status_t HSA_API
    hsa_iterate_agents(hsa_status_t (*callback)(hsa_agent_t agent, void* data),
                       void* data) {
  IS_OPEN();
  IS_BAD_PTR(callback);
  return core::Runtime::runtime_singleton_->IterateAgent(callback, data);
}

hsa_status_t HSA_API hsa_agent_get_info(hsa_agent_t agent_handle,
                                        hsa_agent_info_t attribute,
                                        void* value) {
  IS_OPEN();
  IS_BAD_PTR(value);
  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);
  return agent->GetInfo(attribute, value);
}

hsa_status_t HSA_API hsa_agent_get_exception_policies(hsa_agent_t agent,
                                                      hsa_profile_t profile,
                                                      uint16_t* mask) {
  // TODO: not implemented yet.
  return HSA_STATUS_ERROR;
}

hsa_status_t HSA_API
    hsa_agent_extension_supported(uint16_t extension, hsa_agent_t agent_handle,
                                  uint16_t version_major,
                                  uint16_t version_minor, bool* result) {
  IS_OPEN();

  if ((result == NULL) || (extension > HSA_EXTENSION_AMD_PROFILER)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  *result = false;

  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);

  if (agent->device_type() == core::Agent::kAmdGpuDevice) {
    uint16_t agent_version_major = 0;
    hsa_status_t status =
        agent->GetInfo(HSA_AGENT_INFO_VERSION_MAJOR, &agent_version_major);
    assert(status == HSA_STATUS_SUCCESS);

    if (version_major <= agent_version_major) {
      uint16_t agent_version_minor = 0;
      status =
          agent->GetInfo(HSA_AGENT_INFO_VERSION_MINOR, &agent_version_minor);
      assert(status == HSA_STATUS_SUCCESS);

      if (version_minor <= agent_version_minor) {
        *result = true;
      }
    }
  }

  return HSA_STATUS_SUCCESS;
}

/// @brief Api to create a user mode queue.
///
/// @param agent Hsa Agent which will execute Aql commands
///
/// @param size Size of Queue in terms of Aql packet size
///
/// @param type of Queue Single Writer or Multiple Writer
///
/// @param callback Callback function to register in case Quee
/// encounters an error
///
/// @param service_queue Pointer to a service queue
///
/// @param queue Output parameter updated with a pointer to the
/// queue being created
///
/// @return hsa_status
hsa_status_t HSA_API hsa_queue_create(
    hsa_agent_t agent_handle, uint32_t size, hsa_queue_type_t type,
    void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
    void* data, uint32_t private_segment_size, uint32_t group_segment_size,
    hsa_queue_t** queue) {
  IS_OPEN();

  if ((queue == NULL) || (size == 0) || (!IsPowerOfTwo(size)) ||
      (type < HSA_QUEUE_TYPE_MULTI) || (type > HSA_QUEUE_TYPE_SINGLE)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);

  hsa_queue_type_t agent_queue_type = HSA_QUEUE_TYPE_MULTI;
  hsa_status_t status =
      agent->GetInfo(HSA_AGENT_INFO_QUEUE_TYPE, &agent_queue_type);
  assert(HSA_STATUS_SUCCESS == status);

  if (agent_queue_type == HSA_QUEUE_TYPE_SINGLE &&
      type != HSA_QUEUE_TYPE_SINGLE) {
    return HSA_STATUS_ERROR_INVALID_QUEUE_CREATION;
  }

  // TODO: private_segment_size and group_segment_size.
  core::Queue* cmd_queue = NULL;
  status = agent->QueueCreate(size, type, callback, data, private_segment_size,
                              group_segment_size, &cmd_queue);
  if (cmd_queue != NULL) {
    *queue = core::Queue::Convert(cmd_queue);
  } else {
    *queue = NULL;
  }

  return status;
}

hsa_status_t HSA_API
    hsa_soft_queue_create(hsa_region_t region, uint32_t size,
                          hsa_queue_type_t type, uint32_t features,
                          hsa_signal_t doorbell_signal, hsa_queue_t** queue) {
  IS_OPEN();

  if ((queue == NULL) || (region.handle == 0) ||
      (doorbell_signal.handle == 0) || (size == 0) || (!IsPowerOfTwo(size)) ||
      (type < HSA_QUEUE_TYPE_MULTI) || (type > HSA_QUEUE_TYPE_SINGLE) ||
      (features == 0)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  const core::MemoryRegion* mem_region = core::MemoryRegion::Convert(region);
  IS_VALID(mem_region);

  const core::Signal* signal = core::Signal::Convert(doorbell_signal);
  IS_VALID(signal);

  core::HostQueue* host_queue =
      new core::HostQueue(region, size, type, features, doorbell_signal);

  if (!host_queue->active()) {
    delete host_queue;
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  *queue = core::Queue::Convert(host_queue);

  return HSA_STATUS_SUCCESS;
}

/// @brief Api to destroy a user mode queue
///
/// @param queue Pointer to the queue being destroyed
///
/// @return hsa_status
hsa_status_t HSA_API hsa_queue_destroy(hsa_queue_t* queue) {
  IS_OPEN();
  IS_BAD_PTR(queue);
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  IS_VALID(cmd_queue);
  delete cmd_queue;
  return HSA_STATUS_SUCCESS;
}

/// @brief Api to inactivate a user mode queue
///
/// @param queue Pointer to the queue being inactivated
///
/// @return hsa_status
hsa_status_t HSA_API hsa_queue_inactivate(hsa_queue_t* queue) {
  IS_OPEN();
  IS_BAD_PTR(queue);
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  IS_VALID(cmd_queue);
  cmd_queue->Inactivate();
  return HSA_STATUS_SUCCESS;
}

/// @brief Api to read the Read Index of Queue using Acquire semantics
///
/// @param queue Pointer to the queue whose read index is being read
///
/// @return uint64_t Value of Read index
uint64_t HSA_API hsa_queue_load_read_index_acquire(const hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadReadIndexAcquire();
}

/// @brief Api to read the Read Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose read index is being read
///
/// @return uint64_t Value of Read index
uint64_t HSA_API hsa_queue_load_read_index_relaxed(const hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadReadIndexRelaxed();
}

/// @brief Api to read the Write Index of Queue using Acquire semantics
///
/// @param queue Pointer to the queue whose write index is being read
///
/// @return uint64_t Value of Write index
uint64_t HSA_API hsa_queue_load_write_index_acquire(const hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadWriteIndexAcquire();
}

/// @brief Api to read the Write Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose write index is being read
///
/// @return uint64_t Value of Write index
uint64_t HSA_API hsa_queue_load_write_index_relaxed(const hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadWriteIndexAcquire();
}

/// @brief Api to store the Read Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose read index is being updated
///
/// @param value Value of new read index
void HSA_API hsa_queue_store_read_index_relaxed(const hsa_queue_t* queue,
                                                uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreReadIndexRelaxed(value);
}

/// @brief Api to store the Read Index of Queue using Release semantics
///
/// @param queue Pointer to the queue whose read index is being updated
///
/// @param value Value of new read index
void HSA_API hsa_queue_store_read_index_release(const hsa_queue_t* queue,
                                                uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreReadIndexRelease(value);
}

/// @brief Api to store the Write Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value of new write index
void HSA_API hsa_queue_store_write_index_relaxed(const hsa_queue_t* queue,
                                                 uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreWriteIndexRelaxed(value);
}

/// @brief Api to store the Write Index of Queue using Release semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value of new write index
void HSA_API hsa_queue_store_write_index_release(const hsa_queue_t* queue,
                                                 uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreWriteIndexRelease(value);
}

/// @brief Api to compare and swap the Write Index of Queue using Acquire and
/// Release semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param expected Current value of write index
///
/// @param value Value of new write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(const hsa_queue_t* queue,
                                                   uint64_t expected,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->CasWriteIndexAcqRel(expected, value);
}

/// @brief Api to compare and swap the Write Index of Queue using Acquire
/// Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param expected Current value of write index
///
/// @param value Value of new write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_cas_write_index_acquire(const hsa_queue_t* queue,
                                                   uint64_t expected,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->CasWriteIndexAcquire(expected, value);
}

/// @brief Api to compare and swap the Write Index of Queue using Relaxed
/// Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param expected Current value of write index
///
/// @param value Value of new write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_cas_write_index_relaxed(const hsa_queue_t* queue,
                                                   uint64_t expected,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->CasWriteIndexRelaxed(expected, value);
}

/// @brief Api to compare and swap the Write Index of Queue using Release
/// Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param expected Current value of write index
///
/// @param value Value of new write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_cas_write_index_release(const hsa_queue_t* queue,
                                                   uint64_t expected,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->CasWriteIndexRelease(expected, value);
}

/// @brief Api to Add to the Write Index of Queue using Acquire and Release
/// Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value to add to write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_add_write_index_acq_rel(const hsa_queue_t* queue,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->AddWriteIndexAcqRel(value);
}

/// @brief Api to Add to the Write Index of Queue using Acquire Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value to add to write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_add_write_index_acquire(const hsa_queue_t* queue,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->AddWriteIndexAcquire(value);
}

/// @brief Api to Add to the Write Index of Queue using Relaxed Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value to add to write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_add_write_index_relaxed(const hsa_queue_t* queue,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->AddWriteIndexRelaxed(value);
}

/// @brief Api to Add to the Write Index of Queue using Release Semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value to add to write index
///
/// @return uint64_t Value of write index before the update
uint64_t HSA_API hsa_queue_add_write_index_release(const hsa_queue_t* queue,
                                                   uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->AddWriteIndexRelease(value);
}

//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
hsa_status_t HSA_API hsa_agent_iterate_regions(
    hsa_agent_t agent_handle,
    hsa_status_t (*callback)(hsa_region_t region, void* data), void* data) {
  IS_OPEN();
  IS_BAD_PTR(callback);
  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);
  return agent->IterateRegion(callback, data);
}

hsa_status_t HSA_API hsa_region_get_info(hsa_region_t region,
                                         hsa_region_info_t attribute,
                                         void* value) {
  IS_OPEN();
  IS_BAD_PTR(value);

  const core::MemoryRegion* mem_region = core::MemoryRegion::Convert(region);
  IS_VALID(mem_region);

  return mem_region->GetInfo(attribute, value);
}

hsa_status_t HSA_API hsa_memory_register(void* address, size_t size) {
  IS_OPEN();
  
  if (size == 0 && address != NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

#ifndef __linux__
  if (!core::Runtime::runtime_singleton_->Register(address, size))
    return HSA_STATUS_ERROR;
#endif
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_memory_deregister(void* address, size_t size) {
  IS_OPEN();

#ifndef __linux__
  if(core::Runtime::runtime_singleton_->Deregister(address))
    return HSA_STATUS_SUCCESS;
  return HSA_STATUS_ERROR_INVALID_ARGUMENT;
#endif
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
    hsa_memory_allocate(hsa_region_t region, size_t size, void** ptr) {
  IS_OPEN();
  IS_BAD_PTR(ptr);

  const core::MemoryRegion* mem_region = core::MemoryRegion::Convert(region);
  IS_VALID(mem_region);

  return core::Runtime::runtime_singleton_->AllocateMemory(mem_region, size,
                                                           ptr);
}

hsa_status_t HSA_API hsa_memory_free(void* ptr) {
  IS_OPEN();

  if (ptr == NULL) {
    return HSA_STATUS_SUCCESS;
  }

  return core::Runtime::runtime_singleton_->FreeMemory(ptr);
}

hsa_status_t HSA_API hsa_memory_assign_agent(void* ptr,
                                             hsa_agent_t agent_handle,
                                             hsa_access_permission_t access) {
  IS_OPEN();

  if ((ptr == NULL) || (access < HSA_ACCESS_PERMISSION_RO) ||
      (access > HSA_ACCESS_PERMISSION_RW)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);

  return core::Runtime::runtime_singleton_->AssignMemoryToAgent(ptr, *agent,
                                                                access);
}

hsa_status_t HSA_API hsa_memory_copy(void* dst, const void* src, size_t size) {
  IS_OPEN();

  if (dst == NULL || src == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  if (size == 0) {
    return HSA_STATUS_SUCCESS;
  }

  return core::Runtime::runtime_singleton_->CopyMemory(dst, src, size);
}

//-----------------------------------------------------------------------------
// Signals
//-----------------------------------------------------------------------------

typedef struct {
  bool operator()(const hsa_agent_t& lhs, const hsa_agent_t& rhs) const {
    return lhs.handle < rhs.handle;
  }
} AgentHandleCompare;

hsa_status_t HSA_API
    hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers,
                      const hsa_agent_t* consumers, hsa_signal_t* hsa_signal) {
  IS_OPEN();
  IS_BAD_PTR(hsa_signal);

  core::Signal* ret;

  if (num_consumers > 0) {
    IS_BAD_PTR(consumers);

    // Check for duplicates in consumers.
    std::set<hsa_agent_t, AgentHandleCompare> consumer_set =
        std::set<hsa_agent_t, AgentHandleCompare>(consumers,
                                                  consumers + num_consumers);
    if (consumer_set.size() != num_consumers) {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }

  if (core::g_use_interrupt_wait) {
    ret = new core::InterruptSignal(initial_value);
  } else {
    ret = new core::DefaultSignal(initial_value);
  }
  CHECK_ALLOC(ret);
  *hsa_signal = core::Signal::Convert(ret);
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t hsa_signal) {
  IS_OPEN();

  if (hsa_signal.handle == 0) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  core::Signal* signal = core::Signal::Convert(hsa_signal);
  IS_VALID(signal);
  delete signal;
  return HSA_STATUS_SUCCESS;
}

hsa_signal_value_t HSA_API hsa_signal_load_relaxed(hsa_signal_t hsa_signal) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->LoadRelaxed();
}

hsa_signal_value_t HSA_API hsa_signal_load_acquire(hsa_signal_t hsa_signal) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->LoadAcquire();
}

void HSA_API hsa_signal_store_relaxed(hsa_signal_t hsa_signal,
                                      hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->StoreRelaxed(value);
}

void HSA_API hsa_signal_store_release(hsa_signal_t hsa_signal,
                                      hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->StoreRelease(value);
}

hsa_signal_value_t HSA_API
    hsa_signal_wait_relaxed(hsa_signal_t hsa_signal,
                            hsa_signal_condition_t condition,
                            hsa_signal_value_t compare_value,
                            uint64_t timeout_hint,
                            hsa_wait_state_t wait_state_hint) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->WaitRelaxed(condition, compare_value, timeout_hint,
                             wait_state_hint);
}

hsa_signal_value_t HSA_API
    hsa_signal_wait_acquire(hsa_signal_t hsa_signal,
                            hsa_signal_condition_t condition,
                            hsa_signal_value_t compare_value,
                            uint64_t timeout_hint,
                            hsa_wait_state_t wait_state_hint) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->WaitAcquire(condition, compare_value, timeout_hint,
                             wait_state_hint);
}

void HSA_API
    hsa_signal_and_relaxed(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AndRelaxed(value);
}

void HSA_API
    hsa_signal_and_acquire(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AndAcquire(value);
}

void HSA_API
    hsa_signal_and_release(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AndRelease(value);
}

void HSA_API
    hsa_signal_and_acq_rel(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AndAcqRel(value);
}

void HSA_API
    hsa_signal_or_relaxed(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->OrRelaxed(value);
}

void HSA_API
    hsa_signal_or_acquire(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->OrAcquire(value);
}

void HSA_API
    hsa_signal_or_release(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->OrRelease(value);
}

void HSA_API
    hsa_signal_or_acq_rel(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->OrAcqRel(value);
}

void HSA_API
    hsa_signal_xor_relaxed(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->XorRelaxed(value);
}

void HSA_API
    hsa_signal_xor_acquire(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->XorAcquire(value);
}

void HSA_API
    hsa_signal_xor_release(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->XorRelease(value);
}

void HSA_API
    hsa_signal_xor_acq_rel(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->XorAcqRel(value);
}

void HSA_API
    hsa_signal_add_relaxed(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->AddRelaxed(value);
}

void HSA_API
    hsa_signal_add_acquire(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AddAcquire(value);
}

void HSA_API
    hsa_signal_add_release(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AddRelease(value);
}

void HSA_API
    hsa_signal_add_acq_rel(hsa_signal_t hsa_signal, hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->AddAcqRel(value);
}

void HSA_API hsa_signal_subtract_relaxed(hsa_signal_t hsa_signal,
                                         hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->SubRelaxed(value);
}

void HSA_API hsa_signal_subtract_acquire(hsa_signal_t hsa_signal,
                                         hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->SubAcquire(value);
}

void HSA_API hsa_signal_subtract_release(hsa_signal_t hsa_signal,
                                         hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->SubRelease(value);
}

void HSA_API hsa_signal_subtract_acq_rel(hsa_signal_t hsa_signal,
                                         hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  signal->SubAcqRel(value);
}

hsa_signal_value_t HSA_API
    hsa_signal_exchange_relaxed(hsa_signal_t hsa_signal,
                                hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->ExchRelaxed(value);
}

hsa_signal_value_t HSA_API
    hsa_signal_exchange_acquire(hsa_signal_t hsa_signal,
                                hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->ExchAcquire(value);
}

hsa_signal_value_t HSA_API
    hsa_signal_exchange_release(hsa_signal_t hsa_signal,
                                hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->ExchRelease(value);
}

hsa_signal_value_t HSA_API
    hsa_signal_exchange_acq_rel(hsa_signal_t hsa_signal,
                                hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->ExchAcqRel(value);
}

hsa_signal_value_t HSA_API hsa_signal_cas_relaxed(hsa_signal_t hsa_signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->CasRelaxed(expected, value);
}

hsa_signal_value_t HSA_API hsa_signal_cas_acquire(hsa_signal_t hsa_signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->CasAcquire(expected, value);
}

hsa_signal_value_t HSA_API hsa_signal_cas_release(hsa_signal_t hsa_signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->CasRelease(expected, value);
}

hsa_signal_value_t HSA_API hsa_signal_cas_acq_rel(hsa_signal_t hsa_signal,
                                                  hsa_signal_value_t expected,
                                                  hsa_signal_value_t value) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->CasAcqRel(expected, value);
}

//-----------------------------------------------------------------------------
// Code Object
//-----------------------------------------------------------------------------

hsa_status_t HSA_API hsa_isa_from_name(const char* name, hsa_isa_t* isa) {
  IS_OPEN();
  IS_BAD_PTR(name);
  IS_BAD_PTR(isa);

  return core::loader::Isa::Create(name, isa);
}

hsa_status_t HSA_API hsa_isa_get_info(hsa_isa_t isa, hsa_isa_info_t attribute,
                                      uint32_t index, void* value) {
  IS_OPEN();
  IS_BAD_PTR(value);

  core::loader::Isa* isa_object = core::loader::Isa::Object(isa);
  IS_VALID(isa_object);

  return isa_object->GetInfo(attribute, index, value);
}

hsa_status_t HSA_API hsa_isa_compatible(hsa_isa_t code_object_isa,
                                        hsa_isa_t agent_isa, bool* result) {
  IS_OPEN();
  IS_BAD_PTR(result);

  core::loader::Isa* code_object_isa_object =
      core::loader::Isa::Object(code_object_isa);
  IS_VALID(code_object_isa_object);

  core::loader::Isa* agent_isa_object = core::loader::Isa::Object(agent_isa);
  IS_VALID(agent_isa_object);

  return code_object_isa_object->IsCompatible(*agent_isa_object, result);
}

hsa_status_t HSA_API hsa_code_object_serialize(
    hsa_code_object_t code_object,
    hsa_status_t (*alloc_callback)(size_t size, hsa_callback_data_t data,
                                   void** address),
    hsa_callback_data_t callback_data, const char* options,
    void** serialized_code_object, size_t* serialized_code_object_size) {
  IS_OPEN();
  IS_BAD_PTR(alloc_callback);
  IS_BAD_PTR(serialized_code_object);
  IS_BAD_PTR(serialized_code_object_size);

  return core::loader::SerializeCodeObject(
      code_object, alloc_callback, callback_data, options,
      serialized_code_object, serialized_code_object_size);
}

hsa_status_t HSA_API
    hsa_code_object_deserialize(void* serialized_code_object,
                                size_t serialized_code_object_size,
                                const char* options,
                                hsa_code_object_t* code_object) {
  IS_OPEN();
  IS_BAD_PTR(serialized_code_object);
  IS_BAD_PTR(code_object);

  return core::loader::DeserializeCodeObject(serialized_code_object,
                                             serialized_code_object_size,
                                             options, code_object);
}

hsa_status_t HSA_API hsa_code_object_destroy(hsa_code_object_t code_object) {
  IS_OPEN();
  return core::loader::DestroyCodeObject(code_object);
}

hsa_status_t HSA_API hsa_code_object_get_info(hsa_code_object_t code_object,
                                              hsa_code_object_info_t attribute,
                                              void* value) {
  IS_OPEN();
  IS_BAD_PTR(value);

  return core::loader::GetCodeObjectInfo(code_object, attribute, value);
}

hsa_status_t HSA_API hsa_code_object_get_symbol(hsa_code_object_t code_object,
                                                const char* symbol_name,
                                                hsa_code_symbol_t* symbol) {
  IS_OPEN();
  IS_BAD_PTR(symbol_name);
  IS_BAD_PTR(symbol);

  return core::loader::GetCodeObjectSymbol(code_object, symbol_name, symbol);
}

hsa_status_t HSA_API hsa_code_symbol_get_info(hsa_code_symbol_t code_symbol,
                                              hsa_code_symbol_info_t attribute,
                                              void* value) {
  IS_OPEN();
  IS_BAD_PTR(value);

  return core::loader::GetCodeObjectSymbolInfo(code_symbol, attribute, value);
}

hsa_status_t HSA_API hsa_code_object_iterate_symbols(
    hsa_code_object_t code_object,
    hsa_status_t (*callback)(hsa_code_object_t code_object,
                             hsa_code_symbol_t symbol, void* data),
    void* data) {
  IS_OPEN();
  // TODO.
  return HSA_STATUS_ERROR;
}

//-----------------------------------------------------------------------------
// Executable
//-----------------------------------------------------------------------------

hsa_status_t HSA_API
    hsa_executable_create(hsa_profile_t profile,
                          hsa_executable_state_t executable_state,
                          const char* options, hsa_executable_t* executable) {
  IS_OPEN();
  return core::loader::Executable::Create(profile, executable_state, options,
                                          executable);
}

hsa_status_t HSA_API hsa_executable_destroy(hsa_executable_t executable) {
  IS_OPEN();
  return core::loader::Executable::Destroy(executable);
}

hsa_status_t HSA_API
    hsa_executable_load_code_object(hsa_executable_t executable,
                                    hsa_agent_t agent,
                                    hsa_code_object_t code_object,
                                    const char* options) {
  IS_OPEN();
  core::loader::Executable* exec =
      reinterpret_cast<core::loader::Executable*>(executable.handle);
  if (!exec) {
    return HSA_STATUS_ERROR_INVALID_EXECUTABLE;
  }
  return exec->LoadCodeObject(agent, code_object, options);
}

hsa_status_t HSA_API
    hsa_executable_freeze(hsa_executable_t executable, const char* options) {
  IS_OPEN();
  core::loader::Executable* exec =
      reinterpret_cast<core::loader::Executable*>(executable.handle);
  if (!exec) {
    return HSA_STATUS_ERROR_INVALID_EXECUTABLE;
  }
  return exec->Freeze(options);
}

hsa_status_t HSA_API hsa_executable_get_info(hsa_executable_t executable,
                                             hsa_executable_info_t attribute,
                                             void* value) {
  IS_OPEN();

  if (!value) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  core::loader::Executable* exec =
      reinterpret_cast<core::loader::Executable*>(executable.handle);
  if (!exec) {
    return HSA_STATUS_ERROR_INVALID_EXECUTABLE;
  }
  return exec->GetInfo(attribute, value);
}

hsa_status_t HSA_API
    hsa_executable_global_variable_define(hsa_executable_t executable,
                                          const char* variable_name,
                                          void* address) {
  IS_OPEN();
  // TODO
  return HSA_STATUS_ERROR;
}

hsa_status_t HSA_API
    hsa_executable_agent_global_variable_define(hsa_executable_t executable,
                                                hsa_agent_t agent,
                                                const char* variable_name,
                                                void* address) {
  IS_OPEN();
  // TODO
  return HSA_STATUS_ERROR;
}

hsa_status_t HSA_API
    hsa_executable_readonly_variable_define(hsa_executable_t executable,
                                            hsa_agent_t agent,
                                            const char* variable_name,
                                            void* address) {
  IS_OPEN();
  // TODO
  return HSA_STATUS_ERROR;
}

hsa_status_t HSA_API
    hsa_executable_validate(hsa_executable_t executable, uint32_t* result) {
  IS_OPEN();
  // TODO
  return HSA_STATUS_ERROR;
}

hsa_status_t HSA_API
    hsa_executable_get_symbol(hsa_executable_t executable,
                              const char* module_name, const char* symbol_name,
                              hsa_agent_t agent, int32_t call_convention,
                              hsa_executable_symbol_t* symbol) {
  IS_OPEN();

  if (!symbol_name || !symbol) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  core::loader::Executable* exec =
      reinterpret_cast<core::loader::Executable*>(executable.handle);
  if (!exec) {
    return HSA_STATUS_ERROR_INVALID_EXECUTABLE;
  }
  return exec->GetSymbol(module_name, symbol_name, agent, call_convention,
                         symbol);
}

hsa_status_t HSA_API
    hsa_executable_symbol_get_info(hsa_executable_symbol_t executable_symbol,
                                   hsa_executable_symbol_info_t attribute,
                                   void* value) {
  IS_OPEN();

  if (!value) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return core::loader::GetSymbolInfo(executable_symbol, attribute, value);
}

hsa_status_t HSA_API hsa_executable_iterate_symbols(
    hsa_executable_t executable,
    hsa_status_t (*callback)(hsa_executable_t executable,
                             hsa_executable_symbol_t symbol, void* data),
    void* data) {
  IS_OPEN();

  if (!callback) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  core::loader::Executable* exec =
      reinterpret_cast<core::loader::Executable*>(executable.handle);
  if (!exec) {
    return HSA_STATUS_ERROR_INVALID_EXECUTABLE;
  }
  return exec->IterateSymbols(callback, data);
}

//-----------------------------------------------------------------------------
// Errors
//-----------------------------------------------------------------------------

hsa_status_t HSA_API
    hsa_status_string(hsa_status_t status, const char** status_string) {
  IS_OPEN();
  IS_BAD_PTR(status_string);
  const size_t status_u = static_cast<size_t>(status);
  switch (status_u) {
    case HSA_STATUS_SUCCESS:
      *status_string =
          "HSA_STATUS_SUCCESS: The function has been executed successfully.\n";
      break;
    case HSA_STATUS_INFO_BREAK:
      *status_string =
          "HSA_STATUS_INFO_BREAK: A traversal over a list of "
          "elements has been interrupted by the application before "
          "completing.\n";
      break;
    case HSA_STATUS_ERROR:
      *status_string = "HSA_STATUS_ERROR: A generic error has occurred.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_ARGUMENT:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_ARGUMENT: One of the actual "
          "arguments does not meet a precondition stated in the "
          "documentation of the corresponding formal argument.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_QUEUE_CREATION:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_QUEUE_CREATION: The requested "
          "queue creation is not valid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_ALLOCATION:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_ALLOCATION: The requested "
          "allocation is not valid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_AGENT:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_AGENT: The agent is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_REGION:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_REGION: The memory region is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_SIGNAL:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_SIGNAL: The signal is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_QUEUE:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_QUEUE: The queue is invalid.\n";
      break;
    case HSA_STATUS_ERROR_OUT_OF_RESOURCES:
      *status_string =
          "HSA_STATUS_ERROR_OUT_OF_RESOURCES: The runtime failed to "
          "allocate the necessary resources. This error may also "
          "occur when the core runtime library needs to spawn "
          "threads or create internal OS-specific events.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_PACKET_FORMAT:
      *status_string =
          "HSA_STATUS_ERROR_INVALID_PACKET_FORMAT: The AQL packet "
          "is malformed.\n";
      break;
    case HSA_STATUS_ERROR_RESOURCE_FREE:
      *status_string =
          "HSA_STATUS_ERROR_RESOURCE_FREE: An error has been "
          "detected while releasing a resource.\n";
      break;
    case HSA_STATUS_ERROR_NOT_INITIALIZED:
      *status_string =
          "HSA_STATUS_ERROR_NOT_INITIALIZED: An API other than "
          "hsa_init has been invoked while the reference count of "
          "the HSA runtime is zero.\n";
      break;
    case HSA_STATUS_ERROR_REFCOUNT_OVERFLOW:
      *status_string =
          "HSA_STATUS_ERROR_REFCOUNT_OVERFLOW: The maximum "
          "reference count for the object has been reached.\n";
      break;
    case HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS:
      *status_string =
          "HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS: The arguments passed to "
          "a functions are not compatible.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_INDEX:
      *status_string = "The index is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_ISA:
      *status_string = "The instruction set architecture is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_CODE_OBJECT:
      *status_string = "The code object is invalid.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_EXECUTABLE:
      *status_string = "The executable is invalid.\n";
      break;
    case HSA_STATUS_ERROR_FROZEN_EXECUTABLE:
      *status_string = "The executable is frozen.\n";
      break;
    case HSA_STATUS_ERROR_INVALID_SYMBOL_NAME:
      *status_string = "There is no symbol with the given name.\n";
      break;
    case HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED:
      *status_string = "The variable is already defined.\n";
      break;
    case HSA_STATUS_ERROR_VARIABLE_UNDEFINED:
      *status_string = "The variable is undefined.\n";
      break;
    case HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED:
      *status_string =
          "HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED: Image "
          "format is not supported.\n";
      break;
    case HSA_EXT_STATUS_ERROR_IMAGE_SIZE_UNSUPPORTED:
      *status_string =
          "HSA_EXT_STATUS_ERROR_IMAGE_SIZE_UNSUPPORTED: Image size "
          "is not supported.\n";
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return HSA_STATUS_SUCCESS;
}

}  // end of namespace HSA
