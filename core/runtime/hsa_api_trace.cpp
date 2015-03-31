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

#ifndef HSA_NO_TOOLS_EXTENSION

#include "inc/hsa_api_trace.h"

namespace core
{

	//Define core namespace interfaces - copy of function declarations in hsa.h
	hsa_status_t HSA_API hsa_init();
	hsa_status_t HSA_API hsa_shut_down();
	hsa_status_t HSA_API hsa_system_get_info(hsa_system_info_t attribute, void *value);
	hsa_status_t HSA_API hsa_iterate_agents(hsa_status_t (*callback)(hsa_agent_t agent, void *data),void *data);
	hsa_status_t HSA_API hsa_agent_get_info(hsa_agent_t agent,hsa_agent_info_t attribute,void *value);
	hsa_status_t HSA_API hsa_queue_create(hsa_agent_t agent, size_t size, hsa_queue_type_t type,void (*callback)(hsa_status_t status, hsa_queue_t *queue),const hsa_queue_t *service_queue, hsa_queue_t **queue);
	hsa_status_t HSA_API hsa_queue_destroy(hsa_queue_t *queue);
	hsa_status_t HSA_API hsa_queue_inactivate(hsa_queue_t *queue);
	uint64_t HSA_API hsa_queue_load_read_index_acquire(hsa_queue_t *queue);
	uint64_t HSA_API hsa_queue_load_read_index_relaxed(hsa_queue_t *queue);
	uint64_t HSA_API hsa_queue_load_write_index_acquire(hsa_queue_t *queue);
	uint64_t HSA_API hsa_queue_load_write_index_relaxed(hsa_queue_t *queue);
	void HSA_API hsa_queue_store_write_index_relaxed(hsa_queue_t *queue, uint64_t value);
	void HSA_API hsa_queue_store_write_index_release(hsa_queue_t *queue, uint64_t value);
	uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(hsa_queue_t *queue,uint64_t expected,uint64_t value);
	uint64_t HSA_API hsa_queue_cas_write_index_acquire(hsa_queue_t *queue,uint64_t expected,uint64_t value);
	uint64_t HSA_API hsa_queue_cas_write_index_relaxed(hsa_queue_t *queue,uint64_t expected,uint64_t value);
	uint64_t HSA_API hsa_queue_cas_write_index_release(hsa_queue_t *queue,uint64_t expected,uint64_t value);
	uint64_t HSA_API hsa_queue_add_write_index_acq_rel(hsa_queue_t *queue, uint64_t value);
	uint64_t HSA_API hsa_queue_add_write_index_acquire(hsa_queue_t *queue, uint64_t value);
	uint64_t HSA_API hsa_queue_add_write_index_relaxed(hsa_queue_t *queue, uint64_t value);
	uint64_t HSA_API hsa_queue_add_write_index_release(hsa_queue_t *queue, uint64_t value);
	void HSA_API hsa_queue_store_read_index_relaxed(hsa_queue_t *queue, uint64_t value);
	void HSA_API hsa_queue_store_read_index_release(hsa_queue_t *queue, uint64_t value);
	hsa_status_t HSA_API hsa_agent_iterate_regions(hsa_agent_t agent,hsa_status_t (*callback)(hsa_region_t region, void *data), void *data);
	hsa_status_t HSA_API hsa_region_get_info(hsa_region_t region,hsa_region_info_t attribute,void *value);
	hsa_status_t HSA_API hsa_memory_register(void *address, size_t size);
	hsa_status_t HSA_API hsa_memory_deregister(void *address, size_t size);
	hsa_status_t HSA_API hsa_memory_allocate(hsa_region_t region, size_t size, void **ptr);
	hsa_status_t HSA_API hsa_memory_free(void *ptr);
	hsa_status_t HSA_API hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers,const hsa_agent_t *consumers, hsa_signal_t *signal);
	hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t signal);
	hsa_signal_value_t HSA_API hsa_signal_load_relaxed(hsa_signal_t signal);
	hsa_signal_value_t HSA_API hsa_signal_load_acquire(hsa_signal_t signal);
	void HSA_API hsa_signal_store_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_store_release(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_wait_relaxed(hsa_signal_t signal,hsa_signal_condition_t condition,hsa_signal_value_t compare_value,uint64_t timeout_hint,hsa_wait_expectancy_t wait_expectancy_hint);
	hsa_signal_value_t HSA_API hsa_signal_wait_acquire(hsa_signal_t signal,hsa_signal_condition_t condition,hsa_signal_value_t compare_value,uint64_t timeout_hint,hsa_wait_expectancy_t wait_expectancy_hint);
	void HSA_API hsa_signal_and_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_and_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_and_release(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_and_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_or_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_or_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_or_release(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_or_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_xor_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_xor_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_xor_release(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_xor_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_add_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_add_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_add_release(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_add_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_subtract_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_subtract_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_subtract_release(hsa_signal_t signal, hsa_signal_value_t value);
	void HSA_API hsa_signal_subtract_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_exchange_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_exchange_acquire(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_exchange_release(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_exchange_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_cas_relaxed(hsa_signal_t signal,hsa_signal_value_t expected,hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_cas_acquire(hsa_signal_t signal,hsa_signal_value_t expected,hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_cas_release(hsa_signal_t signal,hsa_signal_value_t expected,hsa_signal_value_t value);
	hsa_signal_value_t HSA_API hsa_signal_cas_acq_rel(hsa_signal_t signal,hsa_signal_value_t expected,hsa_signal_value_t value);
	hsa_status_t HSA_API hsa_status_string(hsa_status_t status, const char **status_string);
	hsa_status_t HSA_API hsa_extension_query(hsa_extension_t extension, int* result);
}

ApiTable hsa_api_table_;

ApiTable::ApiTable()
{
	hsa_init=core::hsa_init;
	hsa_shut_down=core::hsa_shut_down;
	hsa_system_get_info=core::hsa_system_get_info;
	hsa_iterate_agents=core::hsa_iterate_agents;
	hsa_agent_get_info=core::hsa_agent_get_info;
	hsa_queue_create=core::hsa_queue_create;
	hsa_queue_destroy=core::hsa_queue_destroy;
	hsa_queue_inactivate=core::hsa_queue_inactivate;
	hsa_queue_load_read_index_acquire=core::hsa_queue_load_read_index_acquire;
	hsa_queue_load_read_index_relaxed=core::hsa_queue_load_read_index_relaxed;
	hsa_queue_load_write_index_acquire=core::hsa_queue_load_write_index_acquire;
	hsa_queue_load_write_index_relaxed=core::hsa_queue_load_write_index_relaxed;
	hsa_queue_store_write_index_relaxed=core::hsa_queue_store_write_index_relaxed;
	hsa_queue_store_write_index_release=core::hsa_queue_store_write_index_release;
	hsa_queue_cas_write_index_acq_rel=core::hsa_queue_cas_write_index_acq_rel;
	hsa_queue_cas_write_index_acquire=core::hsa_queue_cas_write_index_acquire;
	hsa_queue_cas_write_index_relaxed=core::hsa_queue_cas_write_index_relaxed;
	hsa_queue_cas_write_index_release=core::hsa_queue_cas_write_index_release;
	hsa_queue_add_write_index_acq_rel=core::hsa_queue_add_write_index_acq_rel;
	hsa_queue_add_write_index_acquire=core::hsa_queue_add_write_index_acquire;
	hsa_queue_add_write_index_relaxed=core::hsa_queue_add_write_index_relaxed;
	hsa_queue_add_write_index_release=core::hsa_queue_add_write_index_release;
	hsa_queue_store_read_index_relaxed=core::hsa_queue_store_read_index_relaxed;
	hsa_queue_store_read_index_release=core::hsa_queue_store_read_index_release;
	hsa_agent_iterate_regions=core::hsa_agent_iterate_regions;
	hsa_region_get_info=core::hsa_region_get_info;
	hsa_memory_register=core::hsa_memory_register;
	hsa_memory_deregister=core::hsa_memory_deregister;
	hsa_memory_allocate=core::hsa_memory_allocate;
	hsa_memory_free=core::hsa_memory_free;
	hsa_signal_create=core::hsa_signal_create;
	hsa_signal_destroy=core::hsa_signal_destroy;
	hsa_signal_load_relaxed=core::hsa_signal_load_relaxed;
	hsa_signal_load_acquire=core::hsa_signal_load_acquire;
	hsa_signal_store_relaxed=core::hsa_signal_store_relaxed;
	hsa_signal_store_release=core::hsa_signal_store_release;
	hsa_signal_wait_relaxed=core::hsa_signal_wait_relaxed;
	hsa_signal_wait_acquire=core::hsa_signal_wait_acquire;
	hsa_signal_and_relaxed=core::hsa_signal_and_relaxed;
	hsa_signal_and_acquire=core::hsa_signal_and_acquire;
	hsa_signal_and_release=core::hsa_signal_and_release;
	hsa_signal_and_acq_rel=core::hsa_signal_and_acq_rel;
	hsa_signal_or_relaxed=core::hsa_signal_or_relaxed;
	hsa_signal_or_acquire=core::hsa_signal_or_acquire;
	hsa_signal_or_release=core::hsa_signal_or_release;
	hsa_signal_or_acq_rel=core::hsa_signal_or_acq_rel;
	hsa_signal_xor_relaxed=core::hsa_signal_xor_relaxed;
	hsa_signal_xor_acquire=core::hsa_signal_xor_acquire;
	hsa_signal_xor_release=core::hsa_signal_xor_release;
	hsa_signal_xor_acq_rel=core::hsa_signal_xor_acq_rel;
	hsa_signal_exchange_relaxed=core::hsa_signal_exchange_relaxed;
	hsa_signal_exchange_acquire=core::hsa_signal_exchange_acquire;
	hsa_signal_exchange_release=core::hsa_signal_exchange_release;
	hsa_signal_exchange_acq_rel=core::hsa_signal_exchange_acq_rel;
	hsa_signal_add_relaxed=core::hsa_signal_add_relaxed;
	hsa_signal_add_acquire=core::hsa_signal_add_acquire;
	hsa_signal_add_release=core::hsa_signal_add_release;
	hsa_signal_add_acq_rel=core::hsa_signal_add_acq_rel;
	hsa_signal_subtract_relaxed=core::hsa_signal_subtract_relaxed;
	hsa_signal_subtract_acquire=core::hsa_signal_subtract_acquire;
	hsa_signal_subtract_release=core::hsa_signal_subtract_release;
	hsa_signal_subtract_acq_rel=core::hsa_signal_subtract_acq_rel;
	hsa_signal_cas_relaxed=core::hsa_signal_cas_relaxed;
	hsa_signal_cas_acquire=core::hsa_signal_cas_acquire;
	hsa_signal_cas_release=core::hsa_signal_cas_release;
	hsa_signal_cas_acq_rel=core::hsa_signal_cas_acq_rel;
	hsa_status_string=core::hsa_status_string;
	hsa_extension_query=core::hsa_extension_query;
}

//Exported pass through stubs
hsa_status_t HSA_API hsa_init() { return hsa_api_table_.hsa_init(); }
hsa_status_t HSA_API hsa_shut_down() { return hsa_api_table_.hsa_shut_down(); }
hsa_status_t HSA_API hsa_system_get_info(hsa_system_info_t attribute, void *value) { return hsa_api_table_.hsa_system_get_info(attribute, value); }
hsa_status_t HSA_API hsa_iterate_agents(hsa_status_t (*callback)(hsa_agent_t agent, void *data), void *data) { return hsa_api_table_.hsa_iterate_agents(callback, data); }
hsa_status_t HSA_API hsa_agent_get_info(hsa_agent_t agent, hsa_agent_info_t attribute, void *value) { return hsa_api_table_.hsa_agent_get_info(agent, attribute, value); }
hsa_status_t HSA_API hsa_queue_create(hsa_agent_t agent, size_t size, hsa_queue_type_t type, void (*callback)(hsa_status_t status, hsa_queue_t *queue), const hsa_queue_t *service_queue, hsa_queue_t **queue) { return hsa_api_table_.hsa_queue_create(agent, size, type, callback, service_queue, queue); }
hsa_status_t HSA_API hsa_queue_destroy(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_destroy(queue); }
hsa_status_t HSA_API hsa_queue_inactivate(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_inactivate(queue); }
uint64_t HSA_API hsa_queue_load_read_index_acquire(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_load_read_index_acquire(queue); }
uint64_t HSA_API hsa_queue_load_read_index_relaxed(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_load_read_index_relaxed(queue); }
uint64_t HSA_API hsa_queue_load_write_index_acquire(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_load_write_index_acquire(queue); }
uint64_t HSA_API hsa_queue_load_write_index_relaxed(hsa_queue_t *queue) { return hsa_api_table_.hsa_queue_load_write_index_relaxed(queue); }
void HSA_API hsa_queue_store_write_index_relaxed(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_store_write_index_relaxed(queue, value); }
void HSA_API hsa_queue_store_write_index_release(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_store_write_index_release(queue, value); }
uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(hsa_queue_t *queue, uint64_t expected, uint64_t value) { return hsa_api_table_.hsa_queue_cas_write_index_acq_rel(queue, expected, value); }
uint64_t HSA_API hsa_queue_cas_write_index_acquire(hsa_queue_t *queue, uint64_t expected, uint64_t value) { return hsa_api_table_.hsa_queue_cas_write_index_acquire(queue, expected, value); }
uint64_t HSA_API hsa_queue_cas_write_index_relaxed(hsa_queue_t *queue, uint64_t expected, uint64_t value) { return hsa_api_table_.hsa_queue_cas_write_index_relaxed(queue, expected, value); }
uint64_t HSA_API hsa_queue_cas_write_index_release(hsa_queue_t *queue, uint64_t expected, uint64_t value) { return hsa_api_table_.hsa_queue_cas_write_index_release(queue, expected, value); }
uint64_t HSA_API hsa_queue_add_write_index_acq_rel(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_add_write_index_acq_rel(queue, value); }
uint64_t HSA_API hsa_queue_add_write_index_acquire(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_add_write_index_acquire(queue, value); }
uint64_t HSA_API hsa_queue_add_write_index_relaxed(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_add_write_index_relaxed(queue, value); }
uint64_t HSA_API hsa_queue_add_write_index_release(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_add_write_index_release(queue, value); }
void HSA_API hsa_queue_store_read_index_relaxed(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_store_read_index_relaxed(queue, value); }
void HSA_API hsa_queue_store_read_index_release(hsa_queue_t *queue, uint64_t value) { return hsa_api_table_.hsa_queue_store_read_index_release(queue, value); }
hsa_status_t HSA_API hsa_agent_iterate_regions( hsa_agent_t agent, hsa_status_t (*callback)(hsa_region_t region, void *data), void *data) { return hsa_api_table_.hsa_agent_iterate_regions(agent, callback, data); }
hsa_status_t HSA_API hsa_region_get_info(hsa_region_t region, hsa_region_info_t attribute, void *value) { return hsa_api_table_.hsa_region_get_info(region, attribute, value); }
hsa_status_t HSA_API hsa_memory_register(void *address, size_t size) { return hsa_api_table_.hsa_memory_register(address, size); }
hsa_status_t HSA_API hsa_memory_deregister(void *address, size_t size) { return hsa_api_table_.hsa_memory_deregister(address, size); }
hsa_status_t HSA_API hsa_memory_allocate(hsa_region_t region, size_t size, void **ptr) { return hsa_api_table_.hsa_memory_allocate(region, size, ptr); }
hsa_status_t HSA_API hsa_memory_free(void *ptr) { return hsa_api_table_.hsa_memory_free(ptr); }
hsa_status_t HSA_API hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers, const hsa_agent_t *consumers, hsa_signal_t *signal) { return hsa_api_table_.hsa_signal_create(initial_value, num_consumers, consumers, signal); }
hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t signal) { return hsa_api_table_.hsa_signal_destroy(signal); }
hsa_signal_value_t HSA_API hsa_signal_load_relaxed(hsa_signal_t signal) { return hsa_api_table_.hsa_signal_load_relaxed(signal); }
hsa_signal_value_t HSA_API hsa_signal_load_acquire(hsa_signal_t signal) { return hsa_api_table_.hsa_signal_load_acquire(signal); }
void HSA_API hsa_signal_store_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_store_relaxed(signal, value); }
void HSA_API hsa_signal_store_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_store_release(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_wait_relaxed(hsa_signal_t signal, hsa_signal_condition_t condition, hsa_signal_value_t compare_value, uint64_t timeout_hint, hsa_wait_expectancy_t wait_expectancy_hint) { return hsa_api_table_.hsa_signal_wait_relaxed(signal, condition, compare_value, timeout_hint, wait_expectancy_hint); }
hsa_signal_value_t HSA_API hsa_signal_wait_acquire(hsa_signal_t signal, hsa_signal_condition_t condition, hsa_signal_value_t compare_value, uint64_t timeout_hint, hsa_wait_expectancy_t wait_expectancy_hint) { return hsa_api_table_.hsa_signal_wait_acquire(signal, condition, compare_value, timeout_hint, wait_expectancy_hint); }
void HSA_API hsa_signal_and_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_and_relaxed(signal, value); }
void HSA_API hsa_signal_and_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_and_acquire(signal, value); }
void HSA_API hsa_signal_and_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_and_release(signal, value); }
void HSA_API hsa_signal_and_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_and_acq_rel(signal, value); }
void HSA_API hsa_signal_or_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_or_relaxed(signal, value); }
void HSA_API hsa_signal_or_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_or_acquire(signal, value); }
void HSA_API hsa_signal_or_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_or_release(signal, value); }
void HSA_API hsa_signal_or_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_or_acq_rel(signal, value); }
void HSA_API hsa_signal_xor_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_xor_relaxed(signal, value); }
void HSA_API hsa_signal_xor_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_xor_acquire(signal, value); }
void HSA_API hsa_signal_xor_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_xor_release(signal, value); }
void HSA_API hsa_signal_xor_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_xor_acq_rel(signal, value); }
void HSA_API hsa_signal_add_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_add_relaxed(signal, value); }
void HSA_API hsa_signal_add_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_add_acquire(signal, value); }
void HSA_API hsa_signal_add_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_add_release(signal, value); }
void HSA_API hsa_signal_add_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_add_acq_rel(signal, value); }
void HSA_API hsa_signal_subtract_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_subtract_relaxed(signal, value); }
void HSA_API hsa_signal_subtract_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_subtract_acquire(signal, value); }
void HSA_API hsa_signal_subtract_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_subtract_release(signal, value); }
void HSA_API hsa_signal_subtract_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_subtract_acq_rel(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_exchange_relaxed(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_exchange_relaxed(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_exchange_acquire(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_exchange_acquire(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_exchange_release(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_exchange_release(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_exchange_acq_rel(hsa_signal_t signal, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_exchange_acq_rel(signal, value); }
hsa_signal_value_t HSA_API hsa_signal_cas_relaxed(hsa_signal_t signal, hsa_signal_value_t expected, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_cas_relaxed(signal, expected, value); }
hsa_signal_value_t HSA_API hsa_signal_cas_acquire(hsa_signal_t signal, hsa_signal_value_t expected, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_cas_acquire(signal, expected, value); }
hsa_signal_value_t HSA_API hsa_signal_cas_release(hsa_signal_t signal, hsa_signal_value_t expected, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_cas_release(signal, expected, value); }
hsa_signal_value_t HSA_API hsa_signal_cas_acq_rel(hsa_signal_t signal, hsa_signal_value_t expected, hsa_signal_value_t value) { return hsa_api_table_.hsa_signal_cas_acq_rel(signal, expected, value); }
hsa_status_t HSA_API hsa_status_string(hsa_status_t status, const char **status_string) { return hsa_api_table_.hsa_status_string(status, status_string); }
hsa_status_t HSA_API hsa_extension_query(hsa_extension_t extension, int* result) { return hsa_api_table_.hsa_extension_query(extension, result); }

#endif