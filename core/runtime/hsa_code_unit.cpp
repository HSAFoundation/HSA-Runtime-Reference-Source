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

#include <cassert>
#include <cstring>
#include <gelf.h>
#include <malloc.h>

#include "core/inc/hsa_code_unit.h"
#include "core/inc/runtime.h"

#if defined(_WIN32) || defined(_WIN64)
  #define HCU_ALIGNED_MALLOC(size)  _mm_malloc(size, 256)
  #define HCU_ALIGNED_FREE(pointer) _mm_free(pointer)
#else
  #define HCU_ALIGNED_MALLOC(size)  memalign(256, size)
  #define HCU_ALIGNED_FREE(pointer) free(pointer)
#endif // _WIN32 || _WIN64

namespace core {

/* static */ hsa_status_t HsaCodeUnit::Create(
  HsaCodeUnit **hcu,
  hsa_runtime_caller_t caller,
  const hsa_agent_t *agents,
  size_t agent_count,
  void *serialized_code_unit,
  size_t serialized_code_unit_size,
  const char *options,
  hsa_ext_symbol_value_callback_t symbol_value
) {
  if (NULL == hcu) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  try {
    *hcu = new HsaCodeUnit;
  } catch (const std::bad_alloc) {
    *hcu = NULL;
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  hsa_status_t hsa_error_code = (*hcu)->Create(
    caller,
    agents,
    agent_count,
    serialized_code_unit,
    serialized_code_unit_size,
    options,
    symbol_value
  );
  if (HSA_STATUS_SUCCESS != hsa_error_code) {
    delete *hcu;
    *hcu = NULL;
    return hsa_error_code;
  }

  return HSA_STATUS_SUCCESS;
}

/* static */ hsa_status_t HsaCodeUnit::Destroy(HsaCodeUnit *hcu) {
  if (NULL == hcu) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  hsa_status_t hsa_error_code = hcu->Destroy();
  delete hcu;

  return hsa_error_code;
}

hsa_status_t HsaCodeUnit::Create(
  hsa_runtime_caller_t caller,                  // not used
  const hsa_agent_t *agents,                    // not used
  size_t agent_count,                           // not used
  void *serialized_code_unit,
  size_t serialized_code_unit_size,
  const char *options,                          // not used
  hsa_ext_symbol_value_callback_t symbol_value  // not used
) {
  if (NULL == serialized_code_unit || 0 == serialized_code_unit_size) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  if (EV_NONE == elf_version(EV_CURRENT)) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  Elf *elf_mem = elf_memory(
    reinterpret_cast<byte_t*>(serialized_code_unit), serialized_code_unit_size
  );
  if (NULL == elf_mem) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }
  if (ELF_K_ELF != elf_kind(elf_mem)) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  size_t estsi = 0;
  int eec = elf_getshdrstrndx(elf_mem, &estsi);
  if (0 != eec) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  Elf_Scn *escn = NULL;
  while (NULL != (escn = elf_nextscn(elf_mem, escn))) {
    GElf_Shdr eshdr;
    if (&eshdr != gelf_getshdr(escn, &eshdr)) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    char *esn = NULL;
    if (NULL == (esn = elf_strptr(elf_mem, estsi, eshdr.sh_name))) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    switch (eshdr.sh_type) {
      case SHT_PROGBITS: {
        if (0 == strncmp(esn, ".text", 5)) {
          hsa_status_t hec = ProcessText(elf_mem, eshdr, escn);
          if (HSA_STATUS_SUCCESS != hec) {
            return hec;
          }
        }
        break;
      }
      case SHT_SYMTAB: {
        if (0 == strncmp(esn, ".symtab", 7)) {
          hsa_status_t hec = ProcessSymtab(elf_mem, eshdr, escn);
          if (HSA_STATUS_SUCCESS != hec) {
            return hec;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  elf_end(elf_mem);

  // \todo check if .version and agents are compatible once
  // .version is included in the ELF.

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HsaCodeUnit::Destroy() {
  if (NULL != code_section_) {
    hsa_memory_deregister(code_section_, 0);
    HCU_ALIGNED_FREE(code_section_);
  }
  code_section_ = NULL;
  code_entities_.clear();

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HsaCodeUnit::GetInfo(
  hsa_amd_code_unit_info_t attribute, uint32_t index, void *value
) {
  if (NULL == value) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  switch (attribute) {
    case HSA_EXT_CODE_UNIT_INFO_VERSION: {
      assert(false && "Not supported");
      break;
    }
    case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_COUNT: {
      *((uint32_t*)value) = static_cast<uint32_t>(code_entities_.size());
      break;
    }
    default: {
      if (index >= code_entities_.size()) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
      }

      HsaCodeEntity *hce = &code_entities_[index];
      hsa_amd_kernel_code_t *akc = reinterpret_cast<hsa_amd_kernel_code_t*>(
        code_section_ + hce->offset
      );

      if (HSA_EXT_AMD_CODE_VERSION_MAJOR != akc->amd_code_version_major) {
        // \todo new error code?
        return HSA_STATUS_ERROR;
      }

      switch (attribute) {
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_CODE: {
          *((hsa_amd_code_t*)value) = reinterpret_cast<hsa_amd_code_t>(akc);
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_CODE_TYPE: {
          switch (akc->code_type) {
            case HSA_EXT_CODE_KERNEL: {
              *((hsa_amd_code_type32_t*)value) = HSA_EXT_CODE_TYPE_KERNEL;
              break;
            }
            case HSA_EXT_CODE_INDIRECT_FUNCTION: {
              *((hsa_amd_code_type32_t*)value) = HSA_EXT_CODE_TYPE_INDIRECT_FUNCTION;
              break;
            }
            default: {
              *((hsa_amd_code_type32_t*)value) = HSA_EXT_CODE_TYPE_NONE;
              break;
            }
          }
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_NAME: {
          assert(false && "Not supported");
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_CALL_CONVENTION: {
          // \todo add call convention to amd_kernel_code_t.
          assert(false && "Not supported");
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_GROUP_SEGMENT_SIZE: {
          *((uint32_t*)value) = akc->workgroup_group_segment_byte_size;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_KERNARG_SEGMENT_SIZE: {
          *((uint64_t*)value) = akc->kernarg_segment_byte_size;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_PRIVATE_SEGMENT_SIZE: {
          *((uint32_t*)value) = akc->workitem_private_segment_byte_size;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_PRIVATE_SEGMENT_DYNAMIC_CALL_STACK: {
          *((uint32_t*)value) = (akc->code_properties & HSA_EXT_AMD_CODE_PROPERTY_IS_DYNAMIC_CALLSTACK) >> HSA_EXT_AMD_CODE_PROPERTY_IS_DYNAMIC_CALLSTACK_SHIFT;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_GROUP_SEGMENT_ALIGNMENT: {
          *((hsa_powertwo8_t*)value) = akc->group_segment_alignment;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_KERNARG_SEGMENT_ALIGNMENT: {
          *((hsa_powertwo8_t*)value) = akc->kernarg_segment_alignment;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_PRIVATE_SEGMENT_ALIGNMENT: {
          *((hsa_powertwo8_t*)value) = akc->private_segment_alignment;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_WAVEFRONT_SIZE: {
          *((hsa_powertwo8_t*)value) = akc->wavefront_size;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_WORKGROUP_FBARRIER_COUNT: {
          *((uint32_t*)value) = akc->workgroup_fbarrier_count;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_PROFILE: {
          *((hsa_amd_profile8_t*)value) = akc->hsail_profile;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_MACHINE_MODEL: {
          *((hsa_amd_machine_model8_t*)value) = akc->hsail_machine_model;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_HSAIL_VERSION_MAJOR: {
          *((uint32_t*)value) = akc->hsail_version_major;
          break;
        }
        case HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_HSAIL_VERSION_MINOR: {
          *((uint32_t*)value) = akc->hsail_version_minor;
          break;
        }
        default: {
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
        }
      }
      break;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HsaCodeUnit::ProcessSymtab(
  Elf *elf_mem, const GElf_Shdr &eshdr, Elf_Scn *escn
) {
  assert(NULL != elf_mem);
  assert(SHT_SYMTAB == eshdr.sh_type);
  assert(NULL != escn);

  Elf_Data *est = NULL;
  est = elf_getdata(escn, est);
  if (NULL == est) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  size_t est_count = eshdr.sh_size / eshdr.sh_entsize;

  for (size_t i = 0; i < est_count; ++i) {
    GElf_Sym est_sym;
    if (&est_sym != gelf_getsym(est, (int)i, &est_sym)) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    if (STB_GLOBAL == ELF32_ST_BIND(est_sym.st_info)) {
      char *cen = elf_strptr(elf_mem, eshdr.sh_link, est_sym.st_name);
      if (NULL == cen) {
        // \todo new error code?
        return HSA_STATUS_ERROR;
      }

      try {
        code_entities_.push_back(HsaCodeEntity(cen, est_sym.st_value));
      } catch (const std::bad_alloc) {
        return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
      }
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t HsaCodeUnit::ProcessText(
  Elf *elf_mem, const GElf_Shdr &eshdr, Elf_Scn *escn
) {
  assert(NULL != elf_mem);
  assert(SHT_PROGBITS == eshdr.sh_type);
  assert(NULL != escn);

  Elf_Data *text_section = NULL;
  text_section = elf_getdata(escn, text_section);
  if (NULL == text_section) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  code_section_ = (byte_t*)HCU_ALIGNED_MALLOC(text_section->d_size);
  if (NULL == code_section_) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }
  memset(code_section_, 0x0, text_section->d_size);
  memcpy(code_section_, text_section->d_buf, text_section->d_size);
  hsa_memory_register(code_section_, text_section->d_size);

  return HSA_STATUS_SUCCESS;
}

} // namespace core
