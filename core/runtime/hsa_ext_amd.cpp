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
#include "core/inc/agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/amd_hw_aql_command_processor.h"
#include "core/inc/signal.h"
#include "core/inc/thunk.h"

template <class T>
struct ValidityError;
template <>
struct ValidityError<core::Signal*> {
  enum { value = HSA_STATUS_ERROR_INVALID_SIGNAL };
};

template <>
struct ValidityError<core::Agent*> {
  enum { value = HSA_STATUS_ERROR_INVALID_AGENT };
};

template <>
struct ValidityError<core::MemoryRegion*> {
  enum { value = HSA_STATUS_ERROR_INVALID_REGION };
};

template <>
struct ValidityError<core::Queue*> {
  enum { value = HSA_STATUS_ERROR_INVALID_QUEUE };
};

template <class T>
struct ValidityError<const T*> {
  enum { value = ValidityError<T*>::value };
};

#define IS_BAD_PTR(ptr)                                          \
  do {                                                           \
    if ((ptr) == NULL) return HSA_STATUS_ERROR_INVALID_ARGUMENT; \
  } while (false)

#define IS_VALID(ptr)                                           \
  do {                                                          \
    if ((ptr) == NULL || !(ptr)->IsValid())                     \
      return hsa_status_t(ValidityError<decltype(ptr)>::value); \
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

hsa_status_t HSA_API
hsa_amd_coherency_get_type(hsa_agent_t agent_handle,
                           hsa_amd_coherency_type_t* type) {
  IS_OPEN();

  const core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  IS_BAD_PTR(type);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  const amd::GpuAgentInt* gpu_agent =
      static_cast<const amd::GpuAgentInt*>(agent);

  *type = gpu_agent->current_coherency_type();

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_amd_coherency_set_type(hsa_agent_t agent_handle,
                                                hsa_amd_coherency_type_t type) {
  IS_OPEN();

  core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  if (type < HSA_AMD_COHERENCY_TYPE_COHERENT ||
      type > HSA_AMD_COHERENCY_TYPE_NONCOHERENT) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  amd::GpuAgent* gpu_agent = static_cast<amd::GpuAgent*>(agent);

  if (!gpu_agent->current_coherency_type(type)) {
    return HSA_STATUS_ERROR;
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
hsa_amd_profiling_set_profiler_enabled(hsa_queue_t* queue, int enable) {
  IS_OPEN();

  core::Queue* cmd_queue = core::Queue::Convert(queue);

  IS_VALID(cmd_queue);

  AMD_HSA_BITS_SET(cmd_queue->amd_queue_.queue_properties,
                   AMD_QUEUE_PROPERTIES_ENABLE_PROFILING, (enable != 0));

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
hsa_amd_profiling_get_dispatch_time(hsa_agent_t agent_handle,
                                    hsa_signal_t hsa_signal,
                                    hsa_amd_profiling_dispatch_time_t* time) {
  IS_OPEN();

  IS_BAD_PTR(time);

  core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  core::Signal* signal = core::Signal::Convert(hsa_signal);

  IS_VALID(signal);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  amd::GpuAgentInt* gpu_agent = static_cast<amd::GpuAgentInt*>(agent);

  gpu_agent->TranslateTime(signal, *time);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API
hsa_amd_queue_sdma_create(hsa_agent_t agent_handle, size_t buffer_size,
                          void* buffer_addr, uint64_t* queue_id,
                          uint32_t** read_ptr, uint32_t** write_ptr,
                          uint32_t** doorbell) {
  IS_OPEN();

  static const size_t kPageSize = 4096;

  if (!IsMultipleOf(buffer_size, kPageSize) ||
      !IsMultipleOf(buffer_addr, kPageSize) || queue_id == NULL ||
      read_ptr == NULL || write_ptr == NULL || doorbell == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_QUEUE_CREATION;
  }

  amd::GpuAgentInt* gpu_agent = static_cast<amd::GpuAgentInt*>(agent);

  HsaQueueResource queue_resource = {0};
  const HSA_QUEUE_TYPE kQueueType_ = HSA_QUEUE_SDMA;
  if (HSAKMT_STATUS_SUCCESS !=
      hsaKmtCreateQueue(gpu_agent->node_id(), kQueueType_, 100,
                        HSA_QUEUE_PRIORITY_MAXIMUM, buffer_addr, buffer_size,
                        NULL, &queue_resource)) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  *queue_id = queue_resource.QueueId;
  *read_ptr = queue_resource.Queue_read_ptr;
  *write_ptr = queue_resource.Queue_write_ptr;
  *doorbell = queue_resource.Queue_DoorBell;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_amd_queue_sdma_destroy(uint64_t queue_id) {
  IS_OPEN();

  if (HSAKMT_STATUS_SUCCESS != hsaKmtDestroyQueue(queue_id)) {
    return HSA_STATUS_ERROR_INVALID_QUEUE;
  }

  return HSA_STATUS_SUCCESS;
}

uint32_t HSA_API
hsa_amd_signal_wait_any(uint32_t signal_count, hsa_signal_t* hsa_signals,
                        hsa_signal_condition_t* conds,
                        hsa_signal_value_t* values, uint64_t timeout_hint,
                        hsa_wait_state_t wait_hint,
                        hsa_signal_value_t* satisfying_value) {
  for (uint i = 0; i < signal_count; i++)
    assert(IsValid(core::Signal::Convert(hsa_signals[i])) && "Invalid signal.");

  return core::Signal::WaitAny(signal_count, hsa_signals, conds, values,
                               timeout_hint, wait_hint, satisfying_value);
}

hsa_status_t HSA_API
hsa_amd_signal_async_handler(hsa_signal_t hsa_signal,
                             hsa_signal_condition_t cond,
                             hsa_signal_value_t value,
                             hsa_amd_signal_handler handler, void* arg) {
  IS_OPEN();

  core::Signal* signal = core::Signal::Convert(hsa_signal);
  IS_VALID(signal);
  IS_BAD_PTR(handler);

  return core::Runtime::runtime_singleton_->SetAsyncSignalHandler(
      hsa_signal, cond, value, handler, arg);
}

hsa_status_t HSA_API hsa_amd_queue_cu_set_mask(const hsa_queue_t* queue,
                                               uint32_t num_cu_mask_count,
                                               const uint32_t* cu_mask) {
  IS_OPEN();
  IS_BAD_PTR(cu_mask);

  core::Queue* cmd_queue = core::Queue::Convert(queue);
  IS_VALID(cmd_queue);
  return cmd_queue->SetCUMasking(num_cu_mask_count, cu_mask);
}
