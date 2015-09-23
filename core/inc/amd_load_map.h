////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation (if
// any) (collectively, the "Materials") pursuant to the terms and conditions of
// the Software License Agreement included with the Materials. If you do not
// have a copy of the Software License Agreement, contact your AMD
// representative for a copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND. AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE. THE ENTIRE RISK ASSOCIATED WITH THE USE
// OF THE SOFTWARE IS ASSUMED BY YOU. Some jurisdictions do not allow the
// exclusion of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION: AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD. You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor. Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef AMD_LOAD_MAP_H
#define AMD_LOAD_MAP_H

#include "hsa.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @todo.
enum {
  AMD_EXTENSION_LOAD_MAP = 0x1002
};

/// @todo.
typedef struct amd_loaded_code_object_s {
  uint64_t handle;
} amd_loaded_code_object_t;

/// @todo.
enum amd_loaded_code_object_info_t {
  AMD_LOADED_CODE_OBJECT_INFO_ELF_IMAGE = 0,
  AMD_LOADED_CODE_OBJECT_INFO_ELF_IMAGE_SIZE = 1
};

/// @todo.
typedef struct amd_loaded_segment_s {
  uint64_t handle;
} amd_loaded_segment_t;

/// @todo.
enum amd_loaded_segment_info_t {
  AMD_LOADED_SEGMENT_INFO_TYPE = 0,
  AMD_LOADED_SEGMENT_INFO_ELF_BASE_ADDRESS = 1,
  AMD_LOADED_SEGMENT_INFO_LOAD_BASE_ADDRESS = 2,
  AMD_LOADED_SEGMENT_INFO_SIZE = 3
};

/// @todo.
hsa_status_t amd_executable_load_code_object(
  hsa_executable_t executable,
  hsa_agent_t agent,
  hsa_code_object_t code_object,
  const char *options,
  amd_loaded_code_object_t *loaded_code_object);

/// @brief Invokes @p callback for each available executable in current
/// process.
hsa_status_t amd_iterate_executables(
  hsa_status_t (*callback)(
    hsa_executable_t executable,
    void *data),
  void *data);

/// @brief Invokes @p callback for each loaded code object in specified
/// @p executable.
hsa_status_t amd_executable_iterate_loaded_code_objects(
  hsa_executable_t executable,
  hsa_status_t (*callback)(
    amd_loaded_code_object_t loaded_code_object,
    void *data),
  void *data);

/// @brief Retrieves current value of specified @p loaded_code_object's
/// @p attribute.
hsa_status_t amd_loaded_code_object_get_info(
  amd_loaded_code_object_t loaded_code_object,
  amd_loaded_code_object_info_t attribute,
  void *value);

/// @brief Invokes @p callback for each loaded segment in specified
/// @p loaded_code_object.
hsa_status_t amd_loaded_code_object_iterate_loaded_segments(
  amd_loaded_code_object_t loaded_code_object,
  hsa_status_t (*callback)(
    amd_loaded_segment_t loaded_segment,
    void *data),
  void *data);

/// @brief Retrieves current value of specified @p loaded_segment's
/// @p attribute.
hsa_status_t amd_loaded_segment_get_info(
  amd_loaded_segment_t loaded_segment,
  amd_loaded_segment_info_t attribute,
  void *value);

#define amd_load_map_1_00

typedef struct amd_load_map_1_00_pfn_s {
  hsa_status_t (*amd_executable_load_code_object)(
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    const char *options,
    amd_loaded_code_object_t *loaded_code_object);

  hsa_status_t (*amd_iterate_executables)(
    hsa_status_t (*callback)(
      hsa_executable_t executable,
      void *data),
    void *data);

  hsa_status_t (*amd_executable_iterate_loaded_code_objects)(
    hsa_executable_t executable,
    hsa_status_t (*callback)(
      amd_loaded_code_object_t loaded_code_object,
      void *data),
    void *data);

  hsa_status_t (*amd_loaded_code_object_get_info)(
    amd_loaded_code_object_t loaded_code_object,
    amd_loaded_code_object_info_t attribute,
    void *value);

  hsa_status_t (*amd_loaded_code_object_iterate_loaded_segments)(
    amd_loaded_code_object_t loaded_code_object,
    hsa_status_t (*callback)(
      amd_loaded_segment_t loaded_segment,
      void *data),
    void *data);

  hsa_status_t (*amd_loaded_segment_get_info)(
    amd_loaded_segment_t loaded_segment,
    amd_loaded_segment_info_t attribute,
    void *value);
} amd_load_map_1_00_pfn_t;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // AMD_LOAD_MAP_H
