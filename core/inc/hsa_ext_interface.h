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

#ifndef HSA_RUNTME_CORE_INC_AMD_EXT_INTERFACE_H_
#define HSA_RUNTME_CORE_INC_AMD_EXT_INTERFACE_H_

#include <string>
#include <vector>

#include "inc/hsa_ext_image.h"
#include "inc/hsa_ext_amd.h"
#include "inc/hsa_ext_finalize.h"

#include "core/util/os.h"
#include "core/util/utils.h"

namespace core
{
	class ExtensionEntryPoints
	{
	public:
		typedef decltype(hsa_ext_program_create)* hsa_ext_program_create_t;
		typedef decltype(hsa_ext_program_destroy)* hsa_ext_program_destroy_t;
		typedef decltype(hsa_ext_add_module)* hsa_ext_add_module_t;
		typedef decltype(hsa_ext_finalize_program)* hsa_ext_finalize_program_t;
		typedef decltype(hsa_ext_query_program_agent_id)* hsa_ext_query_program_agent_id_t;
		typedef decltype(hsa_ext_query_program_agent_count)* hsa_ext_query_program_agent_count_t;
		typedef decltype(hsa_ext_query_program_agents)* hsa_ext_query_program_agents_t;
		typedef decltype(hsa_ext_query_program_module_count)* hsa_ext_query_program_module_count_t;
		typedef decltype(hsa_ext_query_program_modules)* hsa_ext_query_program_modules_t;
		typedef decltype(hsa_ext_query_program_brig_module)* hsa_ext_query_program_brig_module_t;
		typedef decltype(hsa_ext_query_call_convention)* hsa_ext_query_call_convention_t;
		typedef decltype(hsa_ext_query_symbol_definition)* hsa_ext_query_symbol_definition_t;
		typedef decltype(hsa_ext_define_program_allocation_global_variable_address)* hsa_ext_define_program_allocation_global_variable_address_t;
		typedef decltype(hsa_ext_query_program_allocation_global_variable_address)* hsa_ext_query_program_allocation_global_variable_address_t;
		typedef decltype(hsa_ext_define_agent_allocation_global_variable_address)* hsa_ext_define_agent_allocation_global_variable_address_t;
		typedef decltype(hsa_ext_query_agent_global_variable_address)* hsa_ext_query_agent_global_variable_address_t;
		typedef decltype(hsa_ext_define_readonly_variable_address)* hsa_ext_define_readonly_variable_address_t;
		typedef decltype(hsa_ext_query_readonly_variable_address)* hsa_ext_query_readonly_variable_address_t;
		typedef decltype(hsa_ext_query_kernel_descriptor_address)* hsa_ext_query_kernel_descriptor_address_t;
		typedef decltype(hsa_ext_query_indirect_function_descriptor_address)* hsa_ext_query_indirect_function_descriptor_address_t;
		typedef decltype(hsa_ext_validate_program)* hsa_ext_validate_program_t;
		typedef decltype(hsa_ext_validate_program_module)* hsa_ext_validate_program_module_t;
		typedef decltype(hsa_ext_serialize_program)* hsa_ext_serialize_program_t;
		typedef decltype(hsa_ext_deserialize_program)* hsa_ext_deserialize_program_t;
		typedef decltype(hsa_ext_extra_query_symbol_definition)* hsa_ext_extra_query_symbol_definition_t;
		typedef decltype(hsa_ext_extra_query_program)* hsa_ext_extra_query_program_t;
		typedef decltype(hsa_ext_image_get_format_capability)* hsa_ext_image_get_format_capability_t;
		typedef decltype(hsa_ext_image_get_info)* hsa_ext_image_get_info_t;
		typedef decltype(hsa_ext_image_create_handle)* hsa_ext_image_create_handle_t;
		typedef decltype(hsa_ext_image_import)* hsa_ext_image_import_t;
		typedef decltype(hsa_ext_image_export)* hsa_ext_image_export_t;
		typedef decltype(hsa_ext_image_copy)* hsa_ext_image_copy_t;
		typedef decltype(hsa_ext_image_clear)* hsa_ext_image_clear_t;
		typedef decltype(hsa_ext_image_destroy_handle)* hsa_ext_image_destroy_handle_t;
		typedef decltype(hsa_ext_sampler_create_handle)* hsa_ext_sampler_create_handle_t;
		typedef decltype(hsa_ext_sampler_destroy_handle)* hsa_ext_sampler_destroy_handle_t;
		typedef decltype(hsa_ext_image_clear_generic_data)* hsa_ext_image_clear_generic_data_t;
		typedef decltype(hsa_ext_image_copy_split_offset)* hsa_ext_image_copy_split_offset_t;
		typedef decltype(hsa_ext_get_image_info_max_dim)* hsa_ext_get_image_info_max_dim_t;

		hsa_ext_program_create_t hsa_ext_program_create;
		hsa_ext_program_destroy_t hsa_ext_program_destroy;
		hsa_ext_add_module_t hsa_ext_add_module;
		hsa_ext_finalize_program_t hsa_ext_finalize_program;
		hsa_ext_query_program_agent_id_t hsa_ext_query_program_agent_id;
		hsa_ext_query_program_agent_count_t hsa_ext_query_program_agent_count;
		hsa_ext_query_program_agents_t hsa_ext_query_program_agents;
		hsa_ext_query_program_module_count_t hsa_ext_query_program_module_count;
		hsa_ext_query_program_modules_t hsa_ext_query_program_modules;
		hsa_ext_query_program_brig_module_t hsa_ext_query_program_brig_module;
		hsa_ext_query_call_convention_t hsa_ext_query_call_convention;
		hsa_ext_query_symbol_definition_t hsa_ext_query_symbol_definition;
		hsa_ext_define_program_allocation_global_variable_address_t hsa_ext_define_program_allocation_global_variable_address;
		hsa_ext_query_program_allocation_global_variable_address_t hsa_ext_query_program_allocation_global_variable_address;
		hsa_ext_define_agent_allocation_global_variable_address_t hsa_ext_define_agent_allocation_global_variable_address;
		hsa_ext_query_agent_global_variable_address_t hsa_ext_query_agent_global_variable_address;
		hsa_ext_define_readonly_variable_address_t hsa_ext_define_readonly_variable_address;
		hsa_ext_query_readonly_variable_address_t hsa_ext_query_readonly_variable_address;
		hsa_ext_query_kernel_descriptor_address_t hsa_ext_query_kernel_descriptor_address;
		hsa_ext_query_indirect_function_descriptor_address_t hsa_ext_query_indirect_function_descriptor_address;
		hsa_ext_validate_program_t hsa_ext_validate_program;
		hsa_ext_validate_program_module_t hsa_ext_validate_program_module;
		hsa_ext_serialize_program_t hsa_ext_serialize_program;
		hsa_ext_deserialize_program_t hsa_ext_deserialize_program;
		hsa_ext_extra_query_symbol_definition_t hsa_ext_extra_query_symbol_definition;
		hsa_ext_extra_query_program_t hsa_ext_extra_query_program;
		hsa_ext_image_get_format_capability_t hsa_ext_image_get_format_capability;
		hsa_ext_image_get_info_t hsa_ext_image_get_info;
		hsa_ext_image_create_handle_t hsa_ext_image_create_handle;
		hsa_ext_image_import_t hsa_ext_image_import;
		hsa_ext_image_export_t hsa_ext_image_export;
		hsa_ext_image_copy_t hsa_ext_image_copy;
		hsa_ext_image_clear_t hsa_ext_image_clear;
		hsa_ext_image_destroy_handle_t hsa_ext_image_destroy_handle;
		hsa_ext_sampler_create_handle_t hsa_ext_sampler_create_handle;
		hsa_ext_sampler_destroy_handle_t hsa_ext_sampler_destroy_handle;
		hsa_ext_image_copy_split_offset_t hsa_ext_image_copy_split_offset;
		hsa_ext_image_clear_generic_data_t hsa_ext_image_clear_generic_data;
		hsa_ext_get_image_info_max_dim_t hsa_ext_get_image_info_max_dim;

		ExtensionEntryPoints();

		bool Load(std::string library_name);
		void Unload();

private:
	    typedef void(*Load_t)();
	    typedef void(*Unload_t)();
		  
	  	std::vector<os::LibHandle> libs_;
		DISALLOW_COPY_AND_ASSIGN(ExtensionEntryPoints);
	};
}

#endif
