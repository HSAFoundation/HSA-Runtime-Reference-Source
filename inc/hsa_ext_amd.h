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

// HSA AMD extension.

#ifndef HSA_RUNTIME_EXT_AMD_H_
#define HSA_RUNTIME_EXT_AMD_H_

#include "hsa.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hsa_amd_status_s {
  HSA_EXT_AMD_STATUS_INFO_HALT_ITERATION = 0x2000,
  HSA_EXT_AMD_STATUS_ERROR_INVALID_OPTION = 0x3000
} hsa_amd_status_t;

typedef enum hsa_amd_agent_info_s {
  HSA_EXT_AMD_AGENT_INFO_DEVICE_ID = HSA_AGENT_INFO_COUNT,
  HSA_EXT_AMD_AGENT_INFO_CACHELINE_SIZE,
  HSA_EXT_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT,
  HSA_EXT_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY,
  HSA_EXT_AMD_AGENT_INFO_DRIVER_NODE_ID,
  HSA_EXT_AMD_AGENT_INFO_MAX_ADDRESS_WATCH_POINTS
} hsa_amd_agent_info_t;

typedef enum hsa_amd_region_info_s {
  HSA_EXT_AMD_REGION_INFO_HOST_ACCESS = HSA_REGION_INFO_COUNT,
  HSA_EXT_AMD_REGION_INFO_BASE
} hsa_amd_region_info_t;

typedef enum hsa_amd_memory_type_s {
  HSA_EXT_AMD_MEMORY_TYPE_COHERENT = 0,
  HSA_EXT_AMD_MEMORY_TYPE_NONCOHERENT = 1
} hsa_amd_memory_type_t;

hsa_status_t HSA_API
    hsa_ext_get_memory_type(hsa_agent_t agent, hsa_amd_memory_type_t* type);

hsa_status_t HSA_API
    hsa_ext_set_memory_type(hsa_agent_t agent, hsa_amd_memory_type_t type);

typedef struct hsa_amd_dispatch_time_s {
  uint64_t start;
  uint64_t end;
} hsa_amd_dispatch_time_t;

hsa_status_t HSA_API hsa_ext_set_profiling(hsa_queue_t* queue, int enable);

hsa_status_t HSA_API hsa_ext_get_dispatch_times(hsa_agent_t agent,
                                                hsa_signal_t signal,
                                                hsa_amd_dispatch_time_t* time);

hsa_status_t HSA_API
    hsa_ext_sdma_queue_create(hsa_agent_t agent, size_t buffer_size,
                              void* buffer_addr, uint64_t* queue_id,
                              uint32_t** read_ptr, uint32_t** write_ptr,
                              uint32_t** doorbell);

hsa_status_t HSA_API
    hsa_ext_sdma_queue_destroy(hsa_agent_t agent, uint64_t queue_id);

//Asyncronous signal handler function.  Return true to resume monitoring the signal and condition, false to stop.
typedef bool(*hsa_ext_signal_handler)(hsa_signal_value_t value, void* arg);
hsa_status_t HSA_API hsa_ext_async_signal_handler(hsa_signal_t signal, hsa_signal_condition_t cond, hsa_signal_value_t value, hsa_ext_signal_handler handler, void* arg);

//Wait for any signal-condition pair to be satisfied.  Returns the index of the satisfying signal, or MAX_INT if error or timout.  satisfying_value will be set to the value of the satisfying signal, may be NULL.  Subject to spurious wakes.
uint32_t HSA_API hsa_ext_signal_wait_any(uint32_t signal_count, hsa_signal_t* signals, hsa_signal_condition_t* conds, hsa_signal_value_t* values, uint64_t timeout_hint, hsa_wait_state_t wait_hint, hsa_signal_value_t* satisfying_value);

#ifdef __cplusplus
}  // end extern "C" block
#endif

#endif  // header guard
