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
    InitTable();
  }

  void ExtensionEntryPoints::InitTable()
  {
    table.hsa_ext_program_create = hsa_ext_null;
    table.hsa_ext_program_destroy = hsa_ext_null;
    table.hsa_ext_program_add_module = hsa_ext_null;
    table.hsa_ext_program_iterate_modules = hsa_ext_null;
    table.hsa_ext_program_get_info = hsa_ext_null;
    table.hsa_ext_program_finalize = hsa_ext_null;
    table.hsa_ext_image_get_capability = hsa_ext_null;
    table.hsa_ext_image_data_get_info = hsa_ext_null;
    table.hsa_ext_image_create = hsa_ext_null;
    table.hsa_ext_image_import = hsa_ext_null;
    table.hsa_ext_image_export = hsa_ext_null;
    table.hsa_ext_image_copy = hsa_ext_null;
    table.hsa_ext_image_clear = hsa_ext_null;
    table.hsa_ext_image_destroy = hsa_ext_null;
    table.hsa_ext_sampler_create = hsa_ext_null;
    table.hsa_ext_sampler_destroy = hsa_ext_null;
    table.hsa_ext_get_image_info_max_dim = hsa_ext_null;
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
    InitTable();
  }

  bool ExtensionEntryPoints::Load(std::string library_name) {
    os::LibHandle lib = os::LoadLib(library_name);
    if (lib == NULL) {
      return false;
    }
    libs_.push_back(lib);

    void* ptr;

    ptr=os::GetExportAddress(lib, "hsa_ext_program_create");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_create==(ExtTable::hsa_ext_program_create_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_create=(ExtTable::hsa_ext_program_create_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_program_destroy");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_destroy==(ExtTable::hsa_ext_program_destroy_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_destroy=(ExtTable::hsa_ext_program_destroy_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_program_add_module");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_add_module==(ExtTable::hsa_ext_program_add_module_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_add_module=(ExtTable::hsa_ext_program_add_module_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_program_iterate_modules");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_iterate_modules==(ExtTable::hsa_ext_program_iterate_modules_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_iterate_modules=(ExtTable::hsa_ext_program_iterate_modules_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_program_get_info");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_get_info==(ExtTable::hsa_ext_program_get_info_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_get_info=(ExtTable::hsa_ext_program_get_info_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_program_finalize");
    if (ptr!=NULL) {
      assert(table.hsa_ext_program_finalize==(ExtTable::hsa_ext_program_finalize_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_program_finalize=(ExtTable::hsa_ext_program_finalize_t)ptr;
    }

    ptr=os::GetExportAddress(lib, "hsa_ext_image_get_capability");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_get_capability == (ExtTable::hsa_ext_image_get_capability_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_get_capability=(ExtTable::hsa_ext_image_get_capability_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_data_get_info");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_data_get_info == (ExtTable::hsa_ext_image_data_get_info_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_data_get_info = (ExtTable::hsa_ext_image_data_get_info_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_create");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_create==(ExtTable::hsa_ext_image_create_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_create=(ExtTable::hsa_ext_image_create_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_import");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_import==(ExtTable::hsa_ext_image_import_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_import=(ExtTable::hsa_ext_image_import_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_export");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_export==(ExtTable::hsa_ext_image_export_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_export=(ExtTable::hsa_ext_image_export_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_copy");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_copy==(ExtTable::hsa_ext_image_copy_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_copy=(ExtTable::hsa_ext_image_copy_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_clear");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_clear==(ExtTable::hsa_ext_image_clear_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_clear=(ExtTable::hsa_ext_image_clear_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_image_destroy");
    if(ptr!=NULL) {
      assert(table.hsa_ext_image_destroy==(ExtTable::hsa_ext_image_destroy_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_image_destroy=(ExtTable::hsa_ext_image_destroy_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_sampler_create");
    if(ptr!=NULL) {
      assert(table.hsa_ext_sampler_create==(ExtTable::hsa_ext_sampler_create_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_sampler_create=(ExtTable::hsa_ext_sampler_create_t)ptr;
    }
    
    ptr=os::GetExportAddress(lib, "hsa_ext_sampler_destroy");
    if(ptr!=NULL) {
      assert(table.hsa_ext_sampler_destroy==(ExtTable::hsa_ext_sampler_destroy_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_sampler_destroy=(ExtTable::hsa_ext_sampler_destroy_t)ptr;
    }
        
    ptr=os::GetExportAddress(lib, "hsa_ext_get_image_info_max_dim");
    if(ptr!=NULL) {
      assert(table.hsa_ext_get_image_info_max_dim==(ExtTableInternal::hsa_ext_get_image_info_max_dim_t) hsa_ext_null && "Duplicate load of extension import.");
      table.hsa_ext_get_image_info_max_dim=(ExtTableInternal::hsa_ext_get_image_info_max_dim_t)ptr;
    }

	ptr = os::GetExportAddress(lib, "Load");
    if (ptr != NULL) {
      ((Load_t)ptr)(&core::hsa_internal_api_table_.table);
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

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_create_t, index>::type
arg_t(0) hsa_ext_program_create(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_create(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_destroy_t, index>::type
arg_t(0) hsa_ext_program_destroy(arg_t(1) arg1)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_destroy(arg1);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_add_module_t, index>::type
arg_t(0) hsa_ext_program_add_module(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_add_module(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_iterate_modules_t, index>::type
arg_t(0) hsa_ext_program_iterate_modules(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_iterate_modules(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_get_info_t, index>::type
arg_t(0) hsa_ext_program_get_info(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_get_info(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_program_finalize_t, index>::type
arg_t(0) hsa_ext_program_finalize(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6, arg_t(7) arg7)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_program_finalize(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_get_capability_t, index>::type
arg_t(0) hsa_ext_image_get_capability(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_get_capability(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_data_get_info_t, index>::type
arg_t(0) hsa_ext_image_data_get_info(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_data_get_info(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_create_t, index>::type
arg_t(0) hsa_ext_image_create(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_create(arg1, arg2, arg3, arg4, arg5);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_import_t, index>::type
arg_t(0) hsa_ext_image_import(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_import(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_export_t, index>::type
arg_t(0) hsa_ext_image_export(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_export(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_copy_t, index>::type
arg_t(0) hsa_ext_image_copy(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4, arg_t(5) arg5, arg_t(6) arg6)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_copy(arg1, arg2, arg3, arg4, arg5, arg6);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_clear_t, index>::type
arg_t(0) hsa_ext_image_clear(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3, arg_t(4) arg4)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_clear(arg1, arg2, arg3, arg4);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_image_destroy_t, index>::type
arg_t(0) hsa_ext_image_destroy(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_image_destroy(arg1, arg2);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_sampler_create_t, index>::type
arg_t(0) hsa_ext_sampler_create(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_sampler_create(arg1, arg2, arg3);
}
#undef arg_t

#define arg_t(index) arg_type<ExtTable::hsa_ext_sampler_destroy_t, index>::type
arg_t(0) hsa_ext_sampler_destroy(arg_t(1) arg1, arg_t(2) arg2)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_sampler_destroy(arg1, arg2);
}
#undef arg_t

//---------------------------------------------------------------------------//
//  Stubs for internal extension functions
//---------------------------------------------------------------------------//

#define arg_t(index) arg_type<core::ExtTableInternal::hsa_ext_get_image_info_max_dim_t, index>::type
arg_t(0) hsa_ext_get_image_info_max_dim(arg_t(1) arg1, arg_t(2) arg2, arg_t(3) arg3)
{
  return core::Runtime::runtime_singleton_->extensions_.table.hsa_ext_get_image_info_max_dim(arg1, arg2, arg3);
}
#undef arg_t
