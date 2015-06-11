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

#include "core/inc/hsa_api_trace_int.h"
#include "core/inc/runtime.h"
#include "core/inc/hsa_table_interface.h"

namespace core {

ApiTable hsa_api_table_;
ApiTable hsa_internal_api_table_;

ApiTable::ApiTable() {
  table.std_exts_ = NULL;
  Reset();
}

void ApiTable::LinkExts(ExtTable* ptr) {
  assert(ptr != NULL && "Invalid extension table linked.");
  extension_backup = *ptr;
  table.std_exts_ = ptr;
}

void ApiTable::Reset() {
  table.hsa_init_fn = HSA::hsa_init;
  table.hsa_shut_down_fn = HSA::hsa_shut_down;
  table.hsa_system_get_info_fn = HSA::hsa_system_get_info;
  table.hsa_system_extension_supported_fn = HSA::hsa_system_extension_supported;
  table.hsa_system_get_extension_table_fn = HSA::hsa_system_get_extension_table;
  table.hsa_iterate_agents_fn = HSA::hsa_iterate_agents;
  table.hsa_agent_get_info_fn = HSA::hsa_agent_get_info;
  table.hsa_agent_get_exception_policies_fn =
      HSA::hsa_agent_get_exception_policies;
  table.hsa_agent_extension_supported_fn = HSA::hsa_agent_extension_supported;
  table.hsa_queue_create_fn = HSA::hsa_queue_create;
  table.hsa_soft_queue_create_fn = HSA::hsa_soft_queue_create;
  table.hsa_queue_destroy_fn = HSA::hsa_queue_destroy;
  table.hsa_queue_inactivate_fn = HSA::hsa_queue_inactivate;
  table.hsa_queue_load_read_index_acquire_fn =
      HSA::hsa_queue_load_read_index_acquire;
  table.hsa_queue_load_read_index_relaxed_fn =
      HSA::hsa_queue_load_read_index_relaxed;
  table.hsa_queue_load_write_index_acquire_fn =
      HSA::hsa_queue_load_write_index_acquire;
  table.hsa_queue_load_write_index_relaxed_fn =
      HSA::hsa_queue_load_write_index_relaxed;
  table.hsa_queue_store_write_index_relaxed_fn =
      HSA::hsa_queue_store_write_index_relaxed;
  table.hsa_queue_store_write_index_release_fn =
      HSA::hsa_queue_store_write_index_release;
  table.hsa_queue_cas_write_index_acq_rel_fn =
      HSA::hsa_queue_cas_write_index_acq_rel;
  table.hsa_queue_cas_write_index_acquire_fn =
      HSA::hsa_queue_cas_write_index_acquire;
  table.hsa_queue_cas_write_index_relaxed_fn =
      HSA::hsa_queue_cas_write_index_relaxed;
  table.hsa_queue_cas_write_index_release_fn =
      HSA::hsa_queue_cas_write_index_release;
  table.hsa_queue_add_write_index_acq_rel_fn =
      HSA::hsa_queue_add_write_index_acq_rel;
  table.hsa_queue_add_write_index_acquire_fn =
      HSA::hsa_queue_add_write_index_acquire;
  table.hsa_queue_add_write_index_relaxed_fn =
      HSA::hsa_queue_add_write_index_relaxed;
  table.hsa_queue_add_write_index_release_fn =
      HSA::hsa_queue_add_write_index_release;
  table.hsa_queue_store_read_index_relaxed_fn =
      HSA::hsa_queue_store_read_index_relaxed;
  table.hsa_queue_store_read_index_release_fn =
      HSA::hsa_queue_store_read_index_release;
  table.hsa_agent_iterate_regions_fn = HSA::hsa_agent_iterate_regions;
  table.hsa_region_get_info_fn = HSA::hsa_region_get_info;
  table.hsa_memory_register_fn = HSA::hsa_memory_register;
  table.hsa_memory_deregister_fn = HSA::hsa_memory_deregister;
  table.hsa_memory_allocate_fn = HSA::hsa_memory_allocate;
  table.hsa_memory_free_fn = HSA::hsa_memory_free;
  table.hsa_memory_copy_fn = HSA::hsa_memory_copy;
  table.hsa_memory_assign_agent_fn = HSA::hsa_memory_assign_agent;
  table.hsa_signal_create_fn = HSA::hsa_signal_create;
  table.hsa_signal_destroy_fn = HSA::hsa_signal_destroy;
  table.hsa_signal_load_relaxed_fn = HSA::hsa_signal_load_relaxed;
  table.hsa_signal_load_acquire_fn = HSA::hsa_signal_load_acquire;
  table.hsa_signal_store_relaxed_fn = HSA::hsa_signal_store_relaxed;
  table.hsa_signal_store_release_fn = HSA::hsa_signal_store_release;
  table.hsa_signal_wait_relaxed_fn = HSA::hsa_signal_wait_relaxed;
  table.hsa_signal_wait_acquire_fn = HSA::hsa_signal_wait_acquire;
  table.hsa_signal_and_relaxed_fn = HSA::hsa_signal_and_relaxed;
  table.hsa_signal_and_acquire_fn = HSA::hsa_signal_and_acquire;
  table.hsa_signal_and_release_fn = HSA::hsa_signal_and_release;
  table.hsa_signal_and_acq_rel_fn = HSA::hsa_signal_and_acq_rel;
  table.hsa_signal_or_relaxed_fn = HSA::hsa_signal_or_relaxed;
  table.hsa_signal_or_acquire_fn = HSA::hsa_signal_or_acquire;
  table.hsa_signal_or_release_fn = HSA::hsa_signal_or_release;
  table.hsa_signal_or_acq_rel_fn = HSA::hsa_signal_or_acq_rel;
  table.hsa_signal_xor_relaxed_fn = HSA::hsa_signal_xor_relaxed;
  table.hsa_signal_xor_acquire_fn = HSA::hsa_signal_xor_acquire;
  table.hsa_signal_xor_release_fn = HSA::hsa_signal_xor_release;
  table.hsa_signal_xor_acq_rel_fn = HSA::hsa_signal_xor_acq_rel;
  table.hsa_signal_exchange_relaxed_fn = HSA::hsa_signal_exchange_relaxed;
  table.hsa_signal_exchange_acquire_fn = HSA::hsa_signal_exchange_acquire;
  table.hsa_signal_exchange_release_fn = HSA::hsa_signal_exchange_release;
  table.hsa_signal_exchange_acq_rel_fn = HSA::hsa_signal_exchange_acq_rel;
  table.hsa_signal_add_relaxed_fn = HSA::hsa_signal_add_relaxed;
  table.hsa_signal_add_acquire_fn = HSA::hsa_signal_add_acquire;
  table.hsa_signal_add_release_fn = HSA::hsa_signal_add_release;
  table.hsa_signal_add_acq_rel_fn = HSA::hsa_signal_add_acq_rel;
  table.hsa_signal_subtract_relaxed_fn = HSA::hsa_signal_subtract_relaxed;
  table.hsa_signal_subtract_acquire_fn = HSA::hsa_signal_subtract_acquire;
  table.hsa_signal_subtract_release_fn = HSA::hsa_signal_subtract_release;
  table.hsa_signal_subtract_acq_rel_fn = HSA::hsa_signal_subtract_acq_rel;
  table.hsa_signal_cas_relaxed_fn = HSA::hsa_signal_cas_relaxed;
  table.hsa_signal_cas_acquire_fn = HSA::hsa_signal_cas_acquire;
  table.hsa_signal_cas_release_fn = HSA::hsa_signal_cas_release;
  table.hsa_signal_cas_acq_rel_fn = HSA::hsa_signal_cas_acq_rel;
  table.hsa_isa_from_name_fn = HSA::hsa_isa_from_name;
  table.hsa_isa_get_info_fn = HSA::hsa_isa_get_info;
  table.hsa_isa_compatible_fn = HSA::hsa_isa_compatible;
  table.hsa_code_object_serialize_fn = HSA::hsa_code_object_serialize;
  table.hsa_code_object_deserialize_fn = HSA::hsa_code_object_deserialize;
  table.hsa_code_object_destroy_fn = HSA::hsa_code_object_destroy;
  table.hsa_code_object_get_info_fn = HSA::hsa_code_object_get_info;
  table.hsa_code_object_get_symbol_fn = HSA::hsa_code_object_get_symbol;
  table.hsa_code_symbol_get_info_fn = HSA::hsa_code_symbol_get_info;
  table.hsa_code_object_iterate_symbols_fn =
      HSA::hsa_code_object_iterate_symbols;
  table.hsa_executable_create_fn = HSA::hsa_executable_create;
  table.hsa_executable_destroy_fn = HSA::hsa_executable_destroy;
  table.hsa_executable_load_code_object_fn =
      HSA::hsa_executable_load_code_object;
  table.hsa_executable_freeze_fn = HSA::hsa_executable_freeze;
  table.hsa_executable_get_info_fn = HSA::hsa_executable_get_info;
  table.hsa_executable_global_variable_define_fn =
      HSA::hsa_executable_global_variable_define;
  table.hsa_executable_agent_global_variable_define_fn =
      HSA::hsa_executable_agent_global_variable_define;
  table.hsa_executable_readonly_variable_define_fn =
      HSA::hsa_executable_readonly_variable_define;
  table.hsa_executable_validate_fn = HSA::hsa_executable_validate;
  table.hsa_executable_get_symbol_fn = HSA::hsa_executable_get_symbol;
  table.hsa_executable_symbol_get_info_fn = HSA::hsa_executable_symbol_get_info;
  table.hsa_executable_iterate_symbols_fn = HSA::hsa_executable_iterate_symbols;
  table.hsa_status_string_fn = HSA::hsa_status_string;

  if (table.std_exts_ != NULL) *table.std_exts_ = extension_backup;
}

class Init {
 public:
  Init() { hsa_table_interface_init(&hsa_api_table_.table); }
};
static Init LinkAtLoad;
}
