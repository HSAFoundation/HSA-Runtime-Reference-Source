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

#include <algorithm>
#include "core/inc/runtime.h"
#include "core/inc/agent.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/signal.h"
#include "core/inc/hsa_code_unit.h"

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

hsa_status_t HSA_API hsa_ext_get_memory_type(hsa_agent_t agent_handle,
                                             hsa_amd_memory_type_t* type) {
  const core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  IS_BAD_PTR(type);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  const amd::GpuAgentInt* gpu_agent = static_cast<const amd::GpuAgentInt*>(agent);

  *type = gpu_agent->memory_type();

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_ext_set_memory_type(hsa_agent_t agent_handle,
                                             hsa_amd_memory_type_t type) {
  core::Agent* agent = core::Agent::Convert(agent_handle);

  IS_VALID(agent);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  amd::GpuAgent* gpu_agent = static_cast<amd::GpuAgent*>(agent);

  if (!gpu_agent->memory_type(type)) {
    return HSA_STATUS_ERROR;
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_ext_set_profiling(hsa_queue_t* queue, int enable) {
  core::Queue* cmd_queue = core::Queue::Convert(queue);

  IS_VALID(cmd_queue);

  cmd_queue->amd_queue_.enable_profiling = (enable != 0);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_ext_get_dispatch_times(hsa_agent_t agent_handle,
                                                hsa_signal_t hsa_signal,
                                                hsa_amd_dispatch_time_t* time) {
  core::Agent* agent = core::Agent::Convert(agent_handle);

  core::Signal* signal = core::Signal::Convert(hsa_signal);

  IS_VALID(agent);

  IS_VALID(signal);

  if (agent->device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR_INVALID_AGENT;
  }

  amd::GpuAgentInt* gpu_agent = static_cast<amd::GpuAgentInt*>(agent);

  gpu_agent->TranslateTime(signal, *time);

  return HSA_STATUS_SUCCESS;
}

//===----------------------------------------------------------------------===//
// HSA Code Unit APIs.                                                        //
//===----------------------------------------------------------------------===//

namespace {
std::vector<hsa_amd_code_unit_t> loaded_code_units;
} // namespace anonymous

hsa_status_t HSA_API hsa_ext_code_unit_load(hsa_runtime_caller_t caller,
                                            const hsa_agent_t *agent,
                                            size_t agent_count,
                                            void *serialized_code_unit,
                                            size_t serialized_code_unit_size,
                                            const char *options,
                                            hsa_ext_symbol_value_callback_t symbol_value,
                                            hsa_amd_code_unit_t *code_unit) {
  if (NULL == code_unit) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  core::HsaCodeUnit *hsa_code_unit;
  hsa_status_t hsa_error_code = core::HsaCodeUnit::Create(
    &hsa_code_unit,
    caller,
    agent,
    agent_count,
    serialized_code_unit,
    serialized_code_unit_size,
    options,
    symbol_value
  );
  if (HSA_STATUS_SUCCESS != hsa_error_code) {
    return hsa_error_code;
  }
  try {
    loaded_code_units.push_back(core::HsaCodeUnit::Handle(hsa_code_unit));
  } catch (const std::bad_alloc) {
    core::HsaCodeUnit::Destroy(hsa_code_unit);
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }
  *code_unit = core::HsaCodeUnit::Handle(hsa_code_unit);
  return HSA_STATUS_SUCCESS;
}

hsa_status_t HSA_API hsa_ext_code_unit_destroy(hsa_amd_code_unit_t code_unit) {
  auto code_unit_pointer = std::find(
    std::begin(loaded_code_units), std::end(loaded_code_units), code_unit
  );
  if (code_unit_pointer == std::end(loaded_code_units)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  *code_unit_pointer = 0;
  return core::HsaCodeUnit::Destroy(core::HsaCodeUnit::Object(code_unit));
}

hsa_status_t HSA_API hsa_ext_code_unit_get_info(hsa_amd_code_unit_t code_unit,
                                                hsa_amd_code_unit_info_t attribute,
                                                uint32_t index,
                                                void *value) {
  auto code_unit_pointer = std::find(
    std::begin(loaded_code_units), std::end(loaded_code_units), code_unit
  );
  if (code_unit_pointer == std::end(loaded_code_units)) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return core::HsaCodeUnit::Object(code_unit)->GetInfo(attribute, index, value);
}

hsa_status_t HSA_API hsa_ext_code_unit_iterator(hsa_runtime_caller_t caller,
                                                hsa_ext_code_unit_iterator_callback_t code_unit_iterator) {
  if (NULL == code_unit_iterator) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  size_t num_loaded_code_units_at_the_moment = loaded_code_units.size();
  for (size_t i = 0; i < num_loaded_code_units_at_the_moment; ++i) {
    hsa_status_t hsa_error_code = code_unit_iterator(
      caller, loaded_code_units[i]
    );
    if (HSA_EXT_STATUS_INFO_HALT_ITERATION == (hsa_amd_status_t)hsa_error_code) {
      return HSA_STATUS_SUCCESS;
    }
    if (HSA_STATUS_SUCCESS != hsa_error_code) {
      return hsa_error_code;
    }
  }
  return HSA_STATUS_SUCCESS;
}
