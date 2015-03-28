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

#ifndef HSA_RUNTIME_INC_HSA_API_TRACE_H
#define HSA_RUNTIME_INC_HSA_API_TRACE_H

#include "hsa.h"
#ifdef AMD_INTERNAL_BUILD
#include "hsa_ext_image.h"
#include "hsa_ext_amd.h"
#include "hsa_ext_finalize.h"
#else
#include "inc/hsa_ext_image.h"
#include "inc/hsa_ext_amd.h"
#include "inc/hsa_ext_finalize.h"
#endif

struct ExtTable {
  typedef decltype(hsa_ext_program_create)* hsa_ext_program_create_t;
  typedef decltype(hsa_ext_program_destroy)* hsa_ext_program_destroy_t;
  typedef decltype(hsa_ext_program_add_module)* hsa_ext_program_add_module_t;
  typedef decltype(hsa_ext_program_iterate_modules)* hsa_ext_program_iterate_modules_t;
  typedef decltype(hsa_ext_program_get_info)* hsa_ext_program_get_info_t;
  typedef decltype(hsa_ext_program_finalize)* hsa_ext_program_finalize_t;
  typedef decltype(
      hsa_ext_image_get_capability)* hsa_ext_image_get_capability_t;
  typedef decltype(hsa_ext_image_data_get_info)* hsa_ext_image_data_get_info_t;
  typedef decltype(hsa_ext_image_create)* hsa_ext_image_create_t;
  typedef decltype(hsa_ext_image_import)* hsa_ext_image_import_t;
  typedef decltype(hsa_ext_image_export)* hsa_ext_image_export_t;
  typedef decltype(hsa_ext_image_copy)* hsa_ext_image_copy_t;
  typedef decltype(hsa_ext_image_clear)* hsa_ext_image_clear_t;
  typedef decltype(
      hsa_ext_image_destroy)* hsa_ext_image_destroy_t;
  typedef decltype(
      hsa_ext_sampler_create)* hsa_ext_sampler_create_t;
  typedef decltype(
      hsa_ext_sampler_destroy)* hsa_ext_sampler_destroy_t;

  hsa_ext_program_create_t  hsa_ext_program_create;
  hsa_ext_program_destroy_t hsa_ext_program_destroy;
  hsa_ext_program_add_module_t hsa_ext_program_add_module;
  hsa_ext_program_iterate_modules_t hsa_ext_program_iterate_modules;
  hsa_ext_program_get_info_t hsa_ext_program_get_info;
  hsa_ext_program_finalize_t hsa_ext_program_finalize;
  hsa_ext_image_get_capability_t hsa_ext_image_get_capability;
  hsa_ext_image_data_get_info_t hsa_ext_image_data_get_info;
  hsa_ext_image_create_t hsa_ext_image_create;
  hsa_ext_image_import_t hsa_ext_image_import;
  hsa_ext_image_export_t hsa_ext_image_export;
  hsa_ext_image_copy_t hsa_ext_image_copy;
  hsa_ext_image_clear_t hsa_ext_image_clear;
  hsa_ext_image_destroy_t hsa_ext_image_destroy;
  hsa_ext_sampler_create_t hsa_ext_sampler_create;
  hsa_ext_sampler_destroy_t hsa_ext_sampler_destroy;
};

struct ApiTable {
  typedef decltype(hsa_init)* hsa_init_t;
  typedef decltype(hsa_shut_down)* hsa_shut_down_t;
  typedef decltype(hsa_system_get_info)* hsa_system_get_info_t;
  typedef decltype(hsa_system_extension_supported)* hsa_system_extension_supported_t;
  typedef decltype(hsa_system_get_extension_table)* hsa_system_get_extension_table_t;
  typedef decltype(hsa_iterate_agents)* hsa_iterate_agents_t;
  typedef decltype(hsa_agent_get_info)* hsa_agent_get_info_t;
  typedef decltype(hsa_queue_create)* hsa_queue_create_t;
  typedef decltype(hsa_soft_queue_create)* hsa_soft_queue_create_t;
  typedef decltype(hsa_queue_destroy)* hsa_queue_destroy_t;
  typedef decltype(hsa_queue_inactivate)* hsa_queue_inactivate_t;
  typedef decltype(
      hsa_queue_load_read_index_acquire)* hsa_queue_load_read_index_acquire_t;
  typedef decltype(
      hsa_queue_load_read_index_relaxed)* hsa_queue_load_read_index_relaxed_t;
  typedef decltype(
      hsa_queue_load_write_index_acquire)* hsa_queue_load_write_index_acquire_t;
  typedef decltype(
      hsa_queue_load_write_index_relaxed)* hsa_queue_load_write_index_relaxed_t;
  typedef decltype(hsa_queue_store_write_index_relaxed)*
      hsa_queue_store_write_index_relaxed_t;
  typedef decltype(hsa_queue_store_write_index_release)*
      hsa_queue_store_write_index_release_t;
  typedef decltype(
      hsa_queue_cas_write_index_acq_rel)* hsa_queue_cas_write_index_acq_rel_t;
  typedef decltype(
      hsa_queue_cas_write_index_acquire)* hsa_queue_cas_write_index_acquire_t;
  typedef decltype(
      hsa_queue_cas_write_index_relaxed)* hsa_queue_cas_write_index_relaxed_t;
  typedef decltype(
      hsa_queue_cas_write_index_release)* hsa_queue_cas_write_index_release_t;
  typedef decltype(
      hsa_queue_add_write_index_acq_rel)* hsa_queue_add_write_index_acq_rel_t;
  typedef decltype(
      hsa_queue_add_write_index_acquire)* hsa_queue_add_write_index_acquire_t;
  typedef decltype(
      hsa_queue_add_write_index_relaxed)* hsa_queue_add_write_index_relaxed_t;
  typedef decltype(
      hsa_queue_add_write_index_release)* hsa_queue_add_write_index_release_t;
  typedef decltype(
      hsa_queue_store_read_index_relaxed)* hsa_queue_store_read_index_relaxed_t;
  typedef decltype(
      hsa_queue_store_read_index_release)* hsa_queue_store_read_index_release_t;
  typedef decltype(hsa_agent_iterate_regions)* hsa_agent_iterate_regions_t;
  typedef decltype(hsa_region_get_info)* hsa_region_get_info_t;
  typedef decltype(hsa_agent_get_exception_policies)* hsa_agent_get_exception_policies_t;
  typedef decltype(hsa_agent_extension_supported)* hsa_agent_extension_supported_t;
  typedef decltype(hsa_memory_register)* hsa_memory_register_t;
  typedef decltype(hsa_memory_deregister)* hsa_memory_deregister_t;
  typedef decltype(hsa_memory_allocate)* hsa_memory_allocate_t;
  typedef decltype(hsa_memory_free)* hsa_memory_free_t;
  typedef decltype(hsa_memory_copy)* hsa_memory_copy_t;
  typedef decltype(hsa_memory_assign_agent)* hsa_memory_assign_agent_t;
  typedef decltype(hsa_signal_create)* hsa_signal_create_t;
  typedef decltype(hsa_signal_destroy)* hsa_signal_destroy_t;
  typedef decltype(hsa_signal_load_relaxed)* hsa_signal_load_relaxed_t;
  typedef decltype(hsa_signal_load_acquire)* hsa_signal_load_acquire_t;
  typedef decltype(hsa_signal_store_relaxed)* hsa_signal_store_relaxed_t;
  typedef decltype(hsa_signal_store_release)* hsa_signal_store_release_t;
  typedef decltype(hsa_signal_wait_relaxed)* hsa_signal_wait_relaxed_t;
  typedef decltype(hsa_signal_wait_acquire)* hsa_signal_wait_acquire_t;
  typedef decltype(hsa_signal_and_relaxed)* hsa_signal_and_relaxed_t;
  typedef decltype(hsa_signal_and_acquire)* hsa_signal_and_acquire_t;
  typedef decltype(hsa_signal_and_release)* hsa_signal_and_release_t;
  typedef decltype(hsa_signal_and_acq_rel)* hsa_signal_and_acq_rel_t;
  typedef decltype(hsa_signal_or_relaxed)* hsa_signal_or_relaxed_t;
  typedef decltype(hsa_signal_or_acquire)* hsa_signal_or_acquire_t;
  typedef decltype(hsa_signal_or_release)* hsa_signal_or_release_t;
  typedef decltype(hsa_signal_or_acq_rel)* hsa_signal_or_acq_rel_t;
  typedef decltype(hsa_signal_xor_relaxed)* hsa_signal_xor_relaxed_t;
  typedef decltype(hsa_signal_xor_acquire)* hsa_signal_xor_acquire_t;
  typedef decltype(hsa_signal_xor_release)* hsa_signal_xor_release_t;
  typedef decltype(hsa_signal_xor_acq_rel)* hsa_signal_xor_acq_rel_t;
  typedef decltype(hsa_signal_exchange_relaxed)* hsa_signal_exchange_relaxed_t;
  typedef decltype(hsa_signal_exchange_acquire)* hsa_signal_exchange_acquire_t;
  typedef decltype(hsa_signal_exchange_release)* hsa_signal_exchange_release_t;
  typedef decltype(hsa_signal_exchange_acq_rel)* hsa_signal_exchange_acq_rel_t;
  typedef decltype(hsa_signal_add_relaxed)* hsa_signal_add_relaxed_t;
  typedef decltype(hsa_signal_add_acquire)* hsa_signal_add_acquire_t;
  typedef decltype(hsa_signal_add_release)* hsa_signal_add_release_t;
  typedef decltype(hsa_signal_add_acq_rel)* hsa_signal_add_acq_rel_t;
  typedef decltype(hsa_signal_subtract_relaxed)* hsa_signal_subtract_relaxed_t;
  typedef decltype(hsa_signal_subtract_acquire)* hsa_signal_subtract_acquire_t;
  typedef decltype(hsa_signal_subtract_release)* hsa_signal_subtract_release_t;
  typedef decltype(hsa_signal_subtract_acq_rel)* hsa_signal_subtract_acq_rel_t;
  typedef decltype(hsa_signal_cas_relaxed)* hsa_signal_cas_relaxed_t;
  typedef decltype(hsa_signal_cas_acquire)* hsa_signal_cas_acquire_t;
  typedef decltype(hsa_signal_cas_release)* hsa_signal_cas_release_t;
  typedef decltype(hsa_signal_cas_acq_rel)* hsa_signal_cas_acq_rel_t;
  typedef decltype(hsa_isa_from_name)* hsa_isa_from_name_t;
  typedef decltype(hsa_isa_get_info)* hsa_isa_get_info_t;
  typedef decltype(hsa_isa_compatible)* hsa_isa_compatible_t;
  typedef decltype(hsa_code_object_serialize)* hsa_code_object_serialize_t;
  typedef decltype(hsa_code_object_deserialize)* hsa_code_object_deserialize_t;
  typedef decltype(hsa_code_object_destroy)* hsa_code_object_destroy_t;
  typedef decltype(hsa_code_object_get_info)* hsa_code_object_get_info_t;
  typedef decltype(hsa_code_object_get_symbol)* hsa_code_object_get_symbol_t;
  typedef decltype(hsa_code_symbol_get_info)* hsa_code_symbol_get_info_t;
  typedef decltype(hsa_code_object_iterate_symbols)* hsa_code_object_iterate_symbols_t;
  typedef decltype(hsa_executable_create)* hsa_executable_create_t;
  typedef decltype(hsa_executable_destroy)* hsa_executable_destroy_t;
  typedef decltype(hsa_executable_load_code_object)* hsa_executable_load_code_object_t;
  typedef decltype(hsa_executable_freeze)* hsa_executable_freeze_t;
  typedef decltype(hsa_executable_get_info)* hsa_executable_get_info_t;
  typedef decltype(hsa_executable_global_variable_define)* hsa_executable_global_variable_define_t;
  typedef decltype(hsa_executable_agent_global_variable_define)* hsa_executable_agent_global_variable_define_t;
  typedef decltype(hsa_executable_readonly_variable_define)* hsa_executable_readonly_variable_define_t;
  typedef decltype(hsa_executable_validate)* hsa_executable_validate_t;
  typedef decltype(hsa_executable_get_symbol)* hsa_executable_get_symbol_t;
  typedef decltype(hsa_executable_symbol_get_info)* hsa_executable_symbol_get_info_t;
  typedef decltype(hsa_executable_iterate_symbols)* hsa_executable_iterate_symbols_t;
  typedef decltype(hsa_status_string)* hsa_status_string_t;

  hsa_init_t hsa_init;
  hsa_shut_down_t hsa_shut_down;
  hsa_system_get_info_t hsa_system_get_info;
  hsa_system_extension_supported_t hsa_system_extension_supported;
  hsa_system_get_extension_table_t hsa_system_get_extension_table;
  hsa_iterate_agents_t hsa_iterate_agents;
  hsa_agent_get_info_t hsa_agent_get_info;
  hsa_agent_get_exception_policies_t hsa_agent_get_exception_policies;
  hsa_agent_extension_supported_t hsa_agent_extension_supported;
  hsa_queue_create_t hsa_queue_create;
  hsa_soft_queue_create_t hsa_soft_queue_create;
  hsa_queue_destroy_t hsa_queue_destroy;
  hsa_queue_inactivate_t hsa_queue_inactivate;
  hsa_queue_load_read_index_acquire_t hsa_queue_load_read_index_acquire;
  hsa_queue_load_read_index_relaxed_t hsa_queue_load_read_index_relaxed;
  hsa_queue_load_write_index_acquire_t hsa_queue_load_write_index_acquire;
  hsa_queue_load_write_index_relaxed_t hsa_queue_load_write_index_relaxed;
  hsa_queue_store_write_index_relaxed_t hsa_queue_store_write_index_relaxed;
  hsa_queue_store_write_index_release_t hsa_queue_store_write_index_release;
  hsa_queue_cas_write_index_acq_rel_t hsa_queue_cas_write_index_acq_rel;
  hsa_queue_cas_write_index_acquire_t hsa_queue_cas_write_index_acquire;
  hsa_queue_cas_write_index_relaxed_t hsa_queue_cas_write_index_relaxed;
  hsa_queue_cas_write_index_release_t hsa_queue_cas_write_index_release;
  hsa_queue_add_write_index_acq_rel_t hsa_queue_add_write_index_acq_rel;
  hsa_queue_add_write_index_acquire_t hsa_queue_add_write_index_acquire;
  hsa_queue_add_write_index_relaxed_t hsa_queue_add_write_index_relaxed;
  hsa_queue_add_write_index_release_t hsa_queue_add_write_index_release;
  hsa_queue_store_read_index_relaxed_t hsa_queue_store_read_index_relaxed;
  hsa_queue_store_read_index_release_t hsa_queue_store_read_index_release;
  hsa_agent_iterate_regions_t hsa_agent_iterate_regions;
  hsa_region_get_info_t hsa_region_get_info;
  hsa_memory_register_t hsa_memory_register;
  hsa_memory_deregister_t hsa_memory_deregister;
  hsa_memory_allocate_t hsa_memory_allocate;
  hsa_memory_free_t hsa_memory_free;
  hsa_memory_copy_t hsa_memory_copy;
  hsa_memory_assign_agent_t hsa_memory_assign_agent;
  hsa_signal_create_t hsa_signal_create;
  hsa_signal_destroy_t hsa_signal_destroy;
  hsa_signal_load_relaxed_t hsa_signal_load_relaxed;
  hsa_signal_load_acquire_t hsa_signal_load_acquire;
  hsa_signal_store_relaxed_t hsa_signal_store_relaxed;
  hsa_signal_store_release_t hsa_signal_store_release;
  hsa_signal_wait_relaxed_t hsa_signal_wait_relaxed;
  hsa_signal_wait_acquire_t hsa_signal_wait_acquire;
  hsa_signal_and_relaxed_t hsa_signal_and_relaxed;
  hsa_signal_and_acquire_t hsa_signal_and_acquire;
  hsa_signal_and_release_t hsa_signal_and_release;
  hsa_signal_and_acq_rel_t hsa_signal_and_acq_rel;
  hsa_signal_or_relaxed_t hsa_signal_or_relaxed;
  hsa_signal_or_acquire_t hsa_signal_or_acquire;
  hsa_signal_or_release_t hsa_signal_or_release;
  hsa_signal_or_acq_rel_t hsa_signal_or_acq_rel;
  hsa_signal_xor_relaxed_t hsa_signal_xor_relaxed;
  hsa_signal_xor_acquire_t hsa_signal_xor_acquire;
  hsa_signal_xor_release_t hsa_signal_xor_release;
  hsa_signal_xor_acq_rel_t hsa_signal_xor_acq_rel;
  hsa_signal_exchange_relaxed_t hsa_signal_exchange_relaxed;
  hsa_signal_exchange_acquire_t hsa_signal_exchange_acquire;
  hsa_signal_exchange_release_t hsa_signal_exchange_release;
  hsa_signal_exchange_acq_rel_t hsa_signal_exchange_acq_rel;
  hsa_signal_add_relaxed_t hsa_signal_add_relaxed;
  hsa_signal_add_acquire_t hsa_signal_add_acquire;
  hsa_signal_add_release_t hsa_signal_add_release;
  hsa_signal_add_acq_rel_t hsa_signal_add_acq_rel;
  hsa_signal_subtract_relaxed_t hsa_signal_subtract_relaxed;
  hsa_signal_subtract_acquire_t hsa_signal_subtract_acquire;
  hsa_signal_subtract_release_t hsa_signal_subtract_release;
  hsa_signal_subtract_acq_rel_t hsa_signal_subtract_acq_rel;
  hsa_signal_cas_relaxed_t hsa_signal_cas_relaxed;
  hsa_signal_cas_acquire_t hsa_signal_cas_acquire;
  hsa_signal_cas_release_t hsa_signal_cas_release;
  hsa_signal_cas_acq_rel_t hsa_signal_cas_acq_rel;
  hsa_isa_from_name_t hsa_isa_from_name;
  hsa_isa_get_info_t hsa_isa_get_info;
  hsa_isa_compatible_t hsa_isa_compatible;
  hsa_code_object_serialize_t hsa_code_object_serialize;
  hsa_code_object_deserialize_t hsa_code_object_deserialize;
  hsa_code_object_destroy_t hsa_code_object_destroy;
  hsa_code_object_get_info_t hsa_code_object_get_info;
  hsa_code_object_get_symbol_t hsa_code_object_get_symbol;
  hsa_code_symbol_get_info_t hsa_code_symbol_get_info;
  hsa_code_object_iterate_symbols_t hsa_code_object_iterate_symbols;
  hsa_executable_create_t hsa_executable_create;
  hsa_executable_destroy_t hsa_executable_destroy;
  hsa_executable_load_code_object_t hsa_executable_load_code_object;
  hsa_executable_freeze_t hsa_executable_freeze;
  hsa_executable_get_info_t hsa_executable_get_info;
  hsa_executable_global_variable_define_t hsa_executable_global_variable_define;
  hsa_executable_agent_global_variable_define_t hsa_executable_agent_global_variable_define;
  hsa_executable_readonly_variable_define_t hsa_executable_readonly_variable_define;
  hsa_executable_validate_t hsa_executable_validate;
  hsa_executable_get_symbol_t hsa_executable_get_symbol;
  hsa_executable_symbol_get_info_t hsa_executable_symbol_get_info;
  hsa_executable_iterate_symbols_t hsa_executable_iterate_symbols;
  hsa_status_string_t hsa_status_string;

  ExtTable* std_exts_;
};

#endif
