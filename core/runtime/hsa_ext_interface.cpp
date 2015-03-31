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

#include "core/inc/hsa_ext_interface.h"

#include "core/inc/runtime.h"
#include "core/util/function_traits.h"

namespace core
{
  //Implementations for missing / unsupported extensions
  template<class T0> static T0 hsa_ext_null() { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1> static T0 hsa_ext_null(T1) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2> static T0 hsa_ext_null(T1, T2) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3> static T0 hsa_ext_null(T1, T2, T3) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4> static T0 hsa_ext_null(T1, T2, T3, T4) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5> static T0 hsa_ext_null(T1, T2, T3, T4, T5) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }
  template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20> static T0 hsa_ext_null(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) { return HSA_STATUS_ERROR_NOT_INITIALIZED; }

  ExtensionEntryPoints::ExtensionEntryPoints()
  {
    hsa_ext_program_create = hsa_ext_null;
    hsa_ext_program_destroy = hsa_ext_null;
    hsa_ext_add_module = hsa_ext_null;
    hsa_ext_finalize_program = hsa_ext_null;
    hsa_ext_query_program_agent_id = hsa_ext_null;
    hsa_ext_query_program_agent_count = hsa_ext_null;
    hsa_ext_query_program_agents = hsa_ext_null;
    hsa_ext_query_program_module_count = hsa_ext_null;
    hsa_ext_query_program_modules = hsa_ext_null;
    hsa_ext_query_program_brig_module = hsa_ext_null;
    hsa_ext_query_call_convention = hsa_ext_null;
    hsa_ext_query_symbol_definition = hsa_ext_null;
    hsa_ext_define_program_allocation_global_variable_address = hsa_ext_null;
    hsa_ext_query_program_allocation_global_variable_address = hsa_ext_null;
    hsa_ext_define_agent_allocation_global_variable_address = hsa_ext_null;
    hsa_ext_query_agent_global_variable_address = hsa_ext_null;
    hsa_ext_define_readonly_variable_address = hsa_ext_null;
    hsa_ext_query_readonly_variable_address = hsa_ext_null;
    hsa_ext_query_kernel_descriptor_address = hsa_ext_null;
    hsa_ext_query_indirect_function_descriptor_address = hsa_ext_null;
    hsa_ext_validate_program = hsa_ext_null;
    hsa_ext_validate_program_module = hsa_ext_null;
    hsa_ext_serialize_program = hsa_ext_null;
    hsa_ext_deserialize_program = hsa_ext_null;
    hsa_ext_extra_query_symbol_definition = hsa_ext_null;
    hsa_ext_extra_query_program = hsa_ext_null;
    hsa_ext_image_get_format_capability = hsa_ext_null;
    hsa_ext_image_get_info = hsa_ext_null;
    hsa_ext_image_create_handle = hsa_ext_null;
    hsa_ext_image_import = hsa_ext_null;
    hsa_ext_image_export = hsa_ext_null;
    hsa_ext_image_copy = hsa_ext_null;
    hsa_ext_image_clear = hsa_ext_null;
    hsa_ext_image_destroy_handle = hsa_ext_null;
    hsa_ext_sampler_create_handle = hsa_ext_null;
    hsa_ext_sampler_destroy_handle = hsa_ext_null;
    hsa_ext_image_clear_generic_data = hsa_ext_null;
    hsa_ext_image_copy_split_offset = hsa_ext_null;
    hsa_ext_get_image_info_max_dim = hsa_ext_null;
  }

  void ExtensionEntryPoints::Unload() {
    for (int i = 0; i < libs_.size(); i++) {
      void* ptr = os::GetExportAddress(libs_[i], "Unload");
      if (ptr) {
        ((Unload_t)ptr)();
      }
    }
    for (int i = 0; i < libs_.size(); i++) {
      os::CloseLib(libs_[i]);
    }
    libs_.clear();
    new (this) ExtensionEntryPoints();
  }

  bool ExtensionEntryPoints::Load(std::string library_name) {
    os::LibHandle lib = os::LoadLib(library_name);
    if (lib == NULL) {
      return false;
    }
    libs_.push_back(lib);

    void* ptr;
    ptr = os::GetExportAddress(lib, "hsa_ext_program_create");
    if (ptr != NULL) {
      assert(hsa_ext_program_create == (hsa_ext_program_create_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_program_create = (hsa_ext_program_create_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_program_destroy");
    if (ptr != NULL) {
      assert(hsa_ext_program_destroy ==
        (hsa_ext_program_destroy_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_program_destroy = (hsa_ext_program_destroy_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_add_module");
    if (ptr != NULL) {
      assert(hsa_ext_add_module == (hsa_ext_add_module_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_add_module = (hsa_ext_add_module_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_finalize_program");
    if (ptr != NULL) {
      assert(hsa_ext_finalize_program ==
        (hsa_ext_finalize_program_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_finalize_program = (hsa_ext_finalize_program_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_agent_id");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_agent_id ==
        (hsa_ext_query_program_agent_id_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_agent_id = (hsa_ext_query_program_agent_id_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_agent_count");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_agent_count ==
        (hsa_ext_query_program_agent_count_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_agent_count =
        (hsa_ext_query_program_agent_count_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_agents");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_agents ==
        (hsa_ext_query_program_agents_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_agents = (hsa_ext_query_program_agents_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_module_count");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_module_count ==
        (hsa_ext_query_program_module_count_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_module_count =
        (hsa_ext_query_program_module_count_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_modules");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_modules ==
        (hsa_ext_query_program_modules_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_modules = (hsa_ext_query_program_modules_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_program_brig_module");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_brig_module ==
        (hsa_ext_query_program_brig_module_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_brig_module =
        (hsa_ext_query_program_brig_module_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_call_convention");
    if (ptr != NULL) {
      assert(hsa_ext_query_call_convention ==
        (hsa_ext_query_call_convention_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_call_convention = (hsa_ext_query_call_convention_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_symbol_definition");
    if (ptr != NULL) {
      assert(hsa_ext_query_symbol_definition ==
        (hsa_ext_query_symbol_definition_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_symbol_definition = (hsa_ext_query_symbol_definition_t)ptr;
    }

    ptr = os::GetExportAddress(
      lib, "hsa_ext_define_program_allocation_global_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_define_program_allocation_global_variable_address ==
        (hsa_ext_define_program_allocation_global_variable_address_t)
        hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_define_program_allocation_global_variable_address =
        (hsa_ext_define_program_allocation_global_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(
      lib, "hsa_ext_query_program_allocation_global_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_query_program_allocation_global_variable_address ==
        (hsa_ext_query_program_allocation_global_variable_address_t)
        hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_program_allocation_global_variable_address =
        (hsa_ext_query_program_allocation_global_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(
      lib, "hsa_ext_define_agent_allocation_global_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_define_agent_allocation_global_variable_address ==
        (hsa_ext_define_agent_allocation_global_variable_address_t)
        hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_define_agent_allocation_global_variable_address =
        (hsa_ext_define_agent_allocation_global_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(lib,
      "hsa_ext_query_agent_global_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_query_agent_global_variable_address ==
        (hsa_ext_query_agent_global_variable_address_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_agent_global_variable_address =
        (hsa_ext_query_agent_global_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_define_readonly_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_define_readonly_variable_address ==
        (hsa_ext_define_readonly_variable_address_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_define_readonly_variable_address =
        (hsa_ext_define_readonly_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_readonly_variable_address");
    if (ptr != NULL) {
      assert(hsa_ext_query_readonly_variable_address ==
        (hsa_ext_query_readonly_variable_address_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_readonly_variable_address =
        (hsa_ext_query_readonly_variable_address_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_query_kernel_descriptor_address");
    if (ptr != NULL) {
      assert(hsa_ext_query_kernel_descriptor_address ==
        (hsa_ext_query_kernel_descriptor_address_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_kernel_descriptor_address =
        (hsa_ext_query_kernel_descriptor_address_t)ptr;
    }

    ptr = os::GetExportAddress(
      lib, "hsa_ext_query_indirect_function_descriptor_address");
    if (ptr != NULL) {
      assert(hsa_ext_query_indirect_function_descriptor_address ==
        (hsa_ext_query_indirect_function_descriptor_address_t)
        hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_query_indirect_function_descriptor_address =
        (hsa_ext_query_indirect_function_descriptor_address_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_validate_program");
    if (ptr != NULL) {
      assert(hsa_ext_validate_program ==
        (hsa_ext_validate_program_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_validate_program = (hsa_ext_validate_program_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_validate_program_module");
    if (ptr != NULL) {
      assert(hsa_ext_validate_program_module ==
        (hsa_ext_validate_program_module_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_validate_program_module = (hsa_ext_validate_program_module_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_serialize_program");
    if (ptr != NULL) {
      assert(hsa_ext_serialize_program ==
        (hsa_ext_serialize_program_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_serialize_program = (hsa_ext_serialize_program_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_deserialize_program");
    if (ptr != NULL) {
      assert(hsa_ext_deserialize_program ==
        (hsa_ext_deserialize_program_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_deserialize_program = (hsa_ext_deserialize_program_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_extra_query_symbol_definition");
    if (ptr != NULL) {
      assert(hsa_ext_extra_query_symbol_definition ==
        (hsa_ext_extra_query_symbol_definition_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_extra_query_symbol_definition =
        (hsa_ext_extra_query_symbol_definition_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_extra_query_program");
    if (ptr != NULL) {
      assert(hsa_ext_extra_query_program ==
        (hsa_ext_extra_query_program_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_extra_query_program = (hsa_ext_extra_query_program_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_get_format_capability");
    if (ptr != NULL) {
      assert(hsa_ext_image_get_format_capability ==
        (hsa_ext_image_get_format_capability_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_get_format_capability =
        (hsa_ext_image_get_format_capability_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_get_info");
    if (ptr != NULL) {
      assert(hsa_ext_image_get_info == (hsa_ext_image_get_info_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_get_info = (hsa_ext_image_get_info_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_create_handle");
    if (ptr != NULL) {
      assert(hsa_ext_image_create_handle ==
        (hsa_ext_image_create_handle_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_create_handle = (hsa_ext_image_create_handle_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_import");
    if (ptr != NULL) {
      assert(hsa_ext_image_import == (hsa_ext_image_import_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_import = (hsa_ext_image_import_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_export");
    if (ptr != NULL) {
      assert(hsa_ext_image_export == (hsa_ext_image_export_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_export = (hsa_ext_image_export_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_copy");
    if (ptr != NULL) {
      assert(hsa_ext_image_copy == (hsa_ext_image_copy_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_copy = (hsa_ext_image_copy_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_copy_split_offset");
    if (ptr != NULL) {
      assert(hsa_ext_image_copy_split_offset ==
        (hsa_ext_image_copy_split_offset_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_copy_split_offset = (hsa_ext_image_copy_split_offset_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_clear");
    if (ptr != NULL) {
      assert(hsa_ext_image_clear == (hsa_ext_image_clear_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_clear = (hsa_ext_image_clear_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_destroy_handle");
    if (ptr != NULL) {
      assert(hsa_ext_image_destroy_handle ==
        (hsa_ext_image_destroy_handle_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_destroy_handle = (hsa_ext_image_destroy_handle_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_sampler_create_handle");
    if (ptr != NULL) {
      assert(hsa_ext_sampler_create_handle ==
        (hsa_ext_sampler_create_handle_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_sampler_create_handle = (hsa_ext_sampler_create_handle_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_sampler_destroy_handle");
    if (ptr != NULL) {
      assert(hsa_ext_sampler_destroy_handle ==
        (hsa_ext_sampler_destroy_handle_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_sampler_destroy_handle = (hsa_ext_sampler_destroy_handle_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_image_clear_generic_data");
    if (ptr != NULL) {
      assert(hsa_ext_image_clear_generic_data ==
        (hsa_ext_image_clear_generic_data_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_image_clear_generic_data =
        (hsa_ext_image_clear_generic_data_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "hsa_ext_get_image_info_max_dim");
    if (ptr != NULL) {
      assert(hsa_ext_get_image_info_max_dim ==
        (hsa_ext_get_image_info_max_dim_t)hsa_ext_null &&
        "Duplicate load of extension import.");
      hsa_ext_get_image_info_max_dim = (hsa_ext_get_image_info_max_dim_t)ptr;
    }

    ptr = os::GetExportAddress(lib, "Load");
    if (ptr != NULL) {
      ((Load_t)ptr)();
    }

    return true;
  }
} // namespace core

//---------------------------------------------------------------------------//
//   Exported extension stub functions
//---------------------------------------------------------------------------//

//Prototype entry point - stays type matched to the implementation.
//#define arg_t(index) arg_type<amd::ExtensionEntryPoints::hsa_ext_program_create_t, index>::type
//arg_t(0) hsa_ext_program_create(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7, arg_t(8) arg8, arg_t(9) arg9, arg_t(10) arg10, arg_t(11) arg11, arg_t(12) arg12, arg_t(13) arg13, arg_t(14) arg14, arg_t(15) arg15, arg_t(16) arg16, arg_t(17) arg17, arg_t(18) arg18, arg_t(19) arg19, arg_t(20) arg20)
//{
// return core::Runtime::runtime_singleton_->extensions_.hsa_ext_program_create(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, arg20);
//}
//#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_program_create_t, index>::type
arg_t(0) hsa_ext_program_create(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_program_create(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_program_destroy_t, index>::type
arg_t(0) hsa_ext_program_destroy(arg_t(1) arg1)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_program_destroy(arg1);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_add_module_t, index>::type
arg_t(0) hsa_ext_add_module(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_add_module(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_finalize_program_t, index>::type
arg_t(0) hsa_ext_finalize_program(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7, arg_t(8) arg8, arg_t(9) arg9)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_finalize_program(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_agent_id_t, index>::type
arg_t(0) hsa_ext_query_program_agent_id(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_agent_id(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_agent_count_t, index>::type
arg_t(0) hsa_ext_query_program_agent_count(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_agent_count(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_agents_t, index>::type
arg_t(0) hsa_ext_query_program_agents(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_agents(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_module_count_t, index>::type
arg_t(0) hsa_ext_query_program_module_count(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_module_count(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_modules_t, index>::type
arg_t(0) hsa_ext_query_program_modules(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_modules(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_brig_module_t, index>::type
arg_t(0) hsa_ext_query_program_brig_module(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_brig_module(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_call_convention_t, index>::type
arg_t(0) hsa_ext_query_call_convention(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_call_convention(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_symbol_definition_t, index>::type
arg_t(0) hsa_ext_query_symbol_definition(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_symbol_definition(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_define_program_allocation_global_variable_address_t, index>::type
arg_t(0) hsa_ext_define_program_allocation_global_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_define_program_allocation_global_variable_address(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_program_allocation_global_variable_address_t, index>::type
arg_t(0) hsa_ext_query_program_allocation_global_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_program_allocation_global_variable_address(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_define_agent_allocation_global_variable_address_t, index>::type
arg_t(0) hsa_ext_define_agent_allocation_global_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_define_agent_allocation_global_variable_address(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_agent_global_variable_address_t, index>::type
arg_t(0) hsa_ext_query_agent_global_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_agent_global_variable_address(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_define_readonly_variable_address_t, index>::type
arg_t(0) hsa_ext_define_readonly_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_define_readonly_variable_address(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_readonly_variable_address_t, index>::type
arg_t(0) hsa_ext_query_readonly_variable_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_readonly_variable_address(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_kernel_descriptor_address_t, index>::type
arg_t(0) hsa_ext_query_kernel_descriptor_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_kernel_descriptor_address(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_query_indirect_function_descriptor_address_t, index>::type
arg_t(0) hsa_ext_query_indirect_function_descriptor_address(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_query_indirect_function_descriptor_address(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_validate_program_t, index>::type
arg_t(0) hsa_ext_validate_program(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_validate_program(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_validate_program_module_t, index>::type
arg_t(0) hsa_ext_validate_program_module(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_validate_program_module(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_serialize_program_t, index>::type
arg_t(0) hsa_ext_serialize_program(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_serialize_program(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_deserialize_program_t, index>::type
arg_t(0) hsa_ext_deserialize_program(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_deserialize_program(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_extra_query_symbol_definition_t, index>::type
arg_t(0) hsa_ext_extra_query_symbol_definition(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_extra_query_symbol_definition(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_extra_query_program_t, index>::type
arg_t(0) hsa_ext_extra_query_program(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_extra_query_program(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_get_format_capability_t, index>::type
arg_t(0) hsa_ext_image_get_format_capability(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_get_format_capability(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_get_info_t, index>::type
arg_t(0) hsa_ext_image_get_info(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_get_info(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_create_handle_t, index>::type
arg_t(0) hsa_ext_image_create_handle(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_create_handle(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_import_t, index>::type
arg_t(0) hsa_ext_image_import(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_import(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_export_t, index>::type
arg_t(0) hsa_ext_image_export(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_export(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_copy_t, index>::type
arg_t(0) hsa_ext_image_copy(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_copy(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_copy_split_offset_t, index>::type
arg_t(0) hsa_ext_image_copy_split_offset(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_copy_split_offset(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_get_image_info_max_dim_t, index>::type
arg_t(0) hsa_ext_get_image_info_max_dim(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_get_image_info_max_dim(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_clear_t, index>::type
arg_t(0) hsa_ext_image_clear(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_clear(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_destroy_handle_t, index>::type
arg_t(0) hsa_ext_image_destroy_handle(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_destroy_handle(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_sampler_create_handle_t, index>::type
arg_t(0) hsa_ext_sampler_create_handle(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_sampler_create_handle(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_sampler_destroy_handle_t, index>::type
arg_t(0) hsa_ext_sampler_destroy_handle(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_sampler_destroy_handle(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<core::ExtensionEntryPoints::hsa_ext_image_clear_generic_data_t, index>::type
arg_t(0) hsa_ext_image_clear_generic_data(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.hsa_ext_image_clear_generic_data(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t
