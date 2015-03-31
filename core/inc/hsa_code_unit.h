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

#ifndef HSA_RUNTIME_CORE_INC_HSA_CODE_UNIT_H_
#define HSA_RUNTIME_CORE_INC_HSA_CODE_UNIT_H_

#include <gelf.h>

#include "core/inc/runtime.h"

namespace core {

typedef char byte_t;
typedef uint32_t offset_t;

struct HsaCodeEntity {
  HsaCodeEntity(char *_name, offset_t _offset): name(_name), offset(_offset) {}

  char *name;
  offset_t offset;
};

class HsaCodeUnit {
public:
  HsaCodeUnit(): code_section_(NULL) {}
  ~HsaCodeUnit() {}

  static hsa_amd_code_unit_t Handle(HsaCodeUnit *hcu) {
    return reinterpret_cast<hsa_amd_code_unit_t>(hcu);
  }

  static HsaCodeUnit *Object(hsa_amd_code_unit_t hcu) {
    return reinterpret_cast<HsaCodeUnit*>(hcu);
  }

  static hsa_status_t Create(
    HsaCodeUnit **hcu,
    hsa_runtime_caller_t caller,
    const hsa_agent_t *agents,
    size_t agent_count,
    void *serialized_code_unit,
    size_t serialized_code_unit_size,
    const char *options,
    hsa_ext_symbol_value_callback_t symbol_value
  );

  static hsa_status_t Destroy(HsaCodeUnit *hcu);

  hsa_status_t Create(
    hsa_runtime_caller_t caller,
    const hsa_agent_t *agents,
    size_t agent_count,
    void *serialized_code_unit,
    size_t serialized_code_unit_size,
    const char *options,
    hsa_ext_symbol_value_callback_t symbol_value
  );

  hsa_status_t Destroy();

  hsa_status_t GetInfo(
    hsa_amd_code_unit_info_t attribute, uint32_t index, void *value
  );

private:
  hsa_status_t ProcessSymtab(
    Elf *elf_mem, const GElf_Shdr &eshdr, Elf_Scn *escn
  );

  hsa_status_t ProcessText(
    Elf *elf_mem, const GElf_Shdr &eshdr, Elf_Scn *escn
  );

  byte_t *code_section_;
  std::vector<HsaCodeEntity> code_entities_;
};

} // namespace core

#endif // HSA_RUNTIME_CORE_INC_HSA_CODE_UNIT_H_
