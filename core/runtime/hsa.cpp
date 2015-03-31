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

#include "core/inc/runtime.h"
#include "core/inc/agent.h"
#include "core/inc/memory_region.h"
#include "core/inc/queue.h"
#include "core/inc/signal.h"
#include "core/inc/default_signal.h"
#include "core/inc/interrupt_signal.h"

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
static_assert(sizeof(hsa_packet_header_t) == 2,
              "AQL packet header has the wrong size.");
static_assert(sizeof(hsa_barrier_packet_t) == sizeof(hsa_dispatch_packet_t),
              "AQL packet definitions have wrong sizes!");
static_assert(sizeof(hsa_barrier_packet_t) ==
                  sizeof(hsa_agent_dispatch_packet_t),
              "AQL packet definitions have wrong sizes!");
static_assert(sizeof(hsa_barrier_packet_t) == 64,
              "AQL packet definitions have wrong sizes!");
#ifdef HSA_LARGE_MODEL
static_assert(sizeof(void*) == 8, "HSA_LARGE_MODEL is set incorrectly!");
#else
static_assert(sizeof(void*) == 4, "HSA_LARGE_MODEL is set incorrectly!");
#endif

#ifndef HSA_NO_TOOLS_EXTENSION
namespace core {
#endif

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

//---------------------------------------------------------------------------//
//  Agent
//---------------------------------------------------------------------------//
hsa_status_t HSA_API
    hsa_iterate_agents(hsa_status_t (*callback)(hsa_agent_t agent, void* data),
                       void* data) {
  IS_OPEN();
  return core::Runtime::runtime_singleton_->IterateAgent(callback, data);
}

hsa_status_t HSA_API hsa_agent_get_info(hsa_agent_t agent_handle,
                                        hsa_agent_info_t attribute,
                                        void* value) {
  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);
  return agent->GetInfo(attribute, value);
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
hsa_status_t HSA_API
    hsa_queue_create(hsa_agent_t agent_handle, size_t size,
                     hsa_queue_type_t type,
                     void (*callback)(hsa_status_t status, hsa_queue_t* queue),
                     const hsa_queue_t* service_queue, hsa_queue_t** queue) {
  IS_BAD_PTR(queue);
  core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);
  core::Queue* cmd_queue;
  hsa_status_t ret =
      agent->QueueCreate(size, type, callback, service_queue, &cmd_queue);
  *queue = core::Queue::Convert(cmd_queue);
  return ret;
}

/// @brief Api to destroy a user mode queue
///
/// @param queue Pointer to the queue being destroyed
///
/// @return hsa_status
hsa_status_t HSA_API hsa_queue_destroy(hsa_queue_t* queue) {
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
uint64_t HSA_API hsa_queue_load_read_index_acquire(hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadReadIndexAcquire();
}

/// @brief Api to read the Read Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose read index is being read
///
/// @return uint64_t Value of Read index
uint64_t HSA_API hsa_queue_load_read_index_relaxed(hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadReadIndexRelaxed();
}

/// @brief Api to read the Write Index of Queue using Acquire semantics
///
/// @param queue Pointer to the queue whose write index is being read
///
/// @return uint64_t Value of Write index
uint64_t HSA_API hsa_queue_load_write_index_acquire(hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadWriteIndexAcquire();
}

/// @brief Api to read the Write Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose write index is being read
///
/// @return uint64_t Value of Write index
uint64_t HSA_API hsa_queue_load_write_index_relaxed(hsa_queue_t* queue) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  return cmd_queue->LoadWriteIndexAcquire();
}

/// @brief Api to store the Read Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose read index is being updated
///
/// @param value Value of new read index
void HSA_API
    hsa_queue_store_read_index_relaxed(hsa_queue_t* queue, uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreReadIndexRelaxed(value);
}

/// @brief Api to store the Read Index of Queue using Release semantics
///
/// @param queue Pointer to the queue whose read index is being updated
///
/// @param value Value of new read index
void HSA_API
    hsa_queue_store_read_index_release(hsa_queue_t* queue, uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreReadIndexRelease(value);
}

/// @brief Api to store the Write Index of Queue using Relaxed semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value of new write index
void HSA_API
    hsa_queue_store_write_index_relaxed(hsa_queue_t* queue, uint64_t value) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);
  assert(IsValid(cmd_queue));
  cmd_queue->StoreWriteIndexRelaxed(value);
}

/// @brief Api to store the Write Index of Queue using Release semantics
///
/// @param queue Pointer to the queue whose write index is being updated
///
/// @param value Value of new write index
void HSA_API
    hsa_queue_store_write_index_release(hsa_queue_t* queue, uint64_t value) {
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
uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(hsa_queue_t* queue,
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
uint64_t HSA_API hsa_queue_cas_write_index_acquire(hsa_queue_t* queue,
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
uint64_t HSA_API hsa_queue_cas_write_index_relaxed(hsa_queue_t* queue,
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
uint64_t HSA_API hsa_queue_cas_write_index_release(hsa_queue_t* queue,
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
uint64_t HSA_API
    hsa_queue_add_write_index_acq_rel(hsa_queue_t* queue, uint64_t value) {
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
uint64_t HSA_API
    hsa_queue_add_write_index_acquire(hsa_queue_t* queue, uint64_t value) {
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
uint64_t HSA_API
    hsa_queue_add_write_index_relaxed(hsa_queue_t* queue, uint64_t value) {
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
uint64_t HSA_API
    hsa_queue_add_write_index_release(hsa_queue_t* queue, uint64_t value) {
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
  const core::Agent* agent = core::Agent::Convert(agent_handle);
  IS_VALID(agent);
  return agent->IterateRegion(callback, data);
}

hsa_status_t HSA_API hsa_region_get_info(hsa_region_t region,
                                         hsa_region_info_t attribute,
                                         void* value) {
  const core::MemoryRegion* mem_region = core::MemoryRegion::Convert(region);

  if (mem_region == NULL) {
    return HSA_STATUS_ERROR_INVALID_REGION;
  }

  return mem_region->GetInfo(attribute, value);
}

hsa_status_t HSA_API hsa_memory_register(void* address, size_t size) {
#ifndef __linux__
  IS_OPEN();
  if (!core::Runtime::runtime_singleton_->Register(address, size))
    return HSA_STATUS_ERROR;
#endif
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_memory_deregister(void* address, size_t size) {
#ifndef __linux__
  IS_OPEN();
  core::Runtime::runtime_singleton_->Deregister(address);
#endif
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
    hsa_memory_allocate(hsa_region_t region, size_t size, void** ptr) {
  IS_BAD_PTR(ptr);

  const core::MemoryRegion* mem_region = core::MemoryRegion::Convert(region);
  IS_VALID(mem_region);

  return core::Runtime::runtime_singleton_->AllocateMemory(mem_region, size,
                                                           ptr);
}

hsa_status_t HSA_API hsa_memory_free(void* ptr) {
  if (ptr == NULL) return HSA_STATUS_SUCCESS;
  return core::Runtime::runtime_singleton_->FreeMemory(ptr);
}

//-----------------------------------------------------------------------------
// Signals
//-----------------------------------------------------------------------------

hsa_status_t HSA_API
    hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers,
                      const hsa_agent_t* consumers, hsa_signal_t* hsa_signal) {
  IS_OPEN();
  IS_BAD_PTR(hsa_signal);
  core::Signal* ret;
  if (num_consumers != 0) IS_BAD_PTR(consumers);
  if (core::g_use_interrupt_wait)
    ret =
        core::InterruptSignal::Create(initial_value, num_consumers, consumers);
  else
    ret = new core::DefaultSignal(initial_value);
  CHECK_ALLOC(ret);
  *hsa_signal = core::Signal::Convert(ret);
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t hsa_signal) {
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
                            hsa_wait_expectancy_t wait_expectancy_hint) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->WaitRelaxed(condition, compare_value, timeout_hint,
                             wait_expectancy_hint);
}

hsa_signal_value_t HSA_API
    hsa_signal_wait_acquire(hsa_signal_t hsa_signal,
                            hsa_signal_condition_t condition,
                            hsa_signal_value_t compare_value,
                            uint64_t timeout_hint,
                            hsa_wait_expectancy_t wait_expectancy_hint) {
  core::Signal* signal = core::Signal::Convert(hsa_signal);
  assert(IsValid(signal));
  return signal->WaitAcquire(condition, compare_value, timeout_hint,
                             wait_expectancy_hint);
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
// Errors
//-----------------------------------------------------------------------------

hsa_status_t HSA_API
    hsa_status_string(hsa_status_t status, const char** status_string) {
  switch (status) {
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
    case HSA_EXT_STATUS_INFO_ALREADY_INITIALIZED:
      *status_string =
          "HSA_EXT_STATUS_INFO_ALREADY_INITIALIZED: An "
          "initialization attempt failed due to prior "
          "initialization.\n";
      break;
    case HSA_EXT_STATUS_INFO_UNRECOGNIZED_OPTIONS:
      *status_string =
          "HSA_EXT_STATUS_INFO_UNRECOGNIZED_OPTIONS: The "
          "finalization options cannot be recognized.\n";
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
    case HSA_EXT_STATUS_ERROR_DIRECTIVE_MISMATCH:
      *status_string =
          "HSA_EXT_STATUS_ERROR_DIRECTIVE_MISMATCH: Mismatch "
          "between a directive in the control directive structure "
          "and in the HSAIL kernel.\n";
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

//-----------------------------------------------------------------------------
// Extensions
//-----------------------------------------------------------------------------

hsa_status_t HSA_API
    hsa_extension_query(hsa_extension_t extension, int* result) {
  IS_OPEN();
  IS_BAD_PTR(result);
  if (core::Runtime::runtime_singleton_->ExtensionQuery(extension))
    *result = 1;
  else
    *result = 0;
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_vendor_extension_query(hsa_extension_t extension,
                                                void* extension_structure,
                                                int* result) {
  assert(false);
  return HSA_STATUS_ERROR;
}

#ifndef HSA_NO_TOOLS_EXTENSION
}  // end of namespace core
#endif
