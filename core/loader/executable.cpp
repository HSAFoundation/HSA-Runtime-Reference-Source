#include "executable.hpp"

#include <cassert>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include "core/inc/hsa_internal.h"

#include <list>
#include <iostream>
#include <gelf.h>

#include <algorithm>
#include "isa.hpp"
#include "core/inc/amd_kernel_code.h"

#if defined(_WIN32) || defined(_WIN64)
  #define NOMINMAX
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif // _WIN32 || _WIN64

/*
FakeElf* Clone(FakeElf *fe) {
  FakeElf* clone = new (std::nothrow) FakeElf;
  if (!clone) {
    return NULL;
  }

  clone->code_section_ =
    new (std::nothrow) byte_t*[fe->executable_symbol_count_];
  if (!clone->code_section_) {
    delete clone;
    return NULL;
  }

  if (fe->debug_sections_) {
    clone->debug_sections_ =
      new (std::nothrow) DebugSection*[fe->executable_symbol_count_];
    if (!clone->debug_sections_) {
      delete [] clone->code_section_;
      delete clone;
      return NULL;
    }
  } else {
    clone->debug_sections_ = NULL;
  }

  clone->executable_symbols_ =
    new (std::nothrow) ExecutableSymbol*[fe->executable_symbol_count_];
  if (!clone->executable_symbols_) {
    delete [] clone->code_section_;
    if (clone->debug_sections_) { delete [] clone->debug_sections_; }
    delete clone;
    return NULL;
  }

  clone->executable_symbol_count_ = fe->executable_symbol_count_;

  for (size_t i = 0; i < fe->executable_symbol_count_; ++i) {
    clone->code_section_[i] = (byte_t*)malloc(
      fe->executable_symbols_[i]->code_size_
    );
    if (!clone) {
      while (0 != i) {
        delete clone->code_section_[--i];
      }
      delete [] clone->executable_symbols_;
      delete [] clone->code_section_;
      if (clone->debug_sections_) { delete [] clone->debug_sections_; }
      delete clone;
      return NULL;
    }
    memcpy(clone->code_section_[i], fe->code_section_[i],
      fe->executable_symbols_[i]->code_size_);

    clone->executable_symbols_[i] = new (std::nothrow) ExecutableSymbol;
    if (!clone->executable_symbols_[i]) {
      while (0 != i) {
        delete clone->executable_symbols_[--i];
      }
      delete [] clone->executable_symbols_;
      delete [] clone->code_section_;
      if (clone->debug_sections_) { delete [] clone->debug_sections_; }
      delete clone;
      return NULL;
    }

    clone->executable_symbols_[i]->name_ = fe->executable_symbols_[i]->name_;
    clone->executable_symbols_[i]->kind_ = fe->executable_symbols_[i]->kind_;
    clone->executable_symbols_[i]->linkage_ =
      fe->executable_symbols_[i]->linkage_;
    clone->executable_symbols_[i]->code_size_ =
      fe->executable_symbols_[i]->code_size_;

    if (fe->debug_sections_) {
      // TODO: hardcode for now...
      clone->debug_sections_[i] =
        new (std::nothrow) DebugSection[2];
      if (!clone->debug_sections_[i]) {
        while (0 != i) {
          delete clone->executable_symbols_[--i];
          if (clone->debug_sections_ && clone->debug_sections_[i]) {
            delete [] clone->debug_sections_[i];
          }
        }
        delete [] clone->executable_symbols_;
        delete [] clone->code_section_;
        if (clone->debug_sections_) { delete [] clone->debug_sections_; }
        delete clone;
        return NULL;
      }

      for (size_t dsi = 0; dsi < 2; ++dsi) {
        clone->debug_sections_[i][dsi].name_= fe->debug_sections_[i][dsi].name_;
        clone->debug_sections_[i][dsi].data_= fe->debug_sections_[i][dsi].data_;
        #if 0
        DEBUG_COUT(i << ": " << fe->debug_sections_[i][dsi].name_);
        DEBUG_COUT(i << ": " << fe->debug_sections_[i][dsi].data_);
        DEBUG_COUT("-----------------");
        #endif
      }
    }
  }

  return clone;
}
*/

#include <cstring>

namespace {

struct SBits{
  unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
};

union UBits {
  SBits bits;
  unsigned char byte;
};

std::size_t RoundUp(
  const std::size_t &in_number,
  const std::size_t &in_multiple
) {
  if (0 == in_multiple) {
    return in_number;
  }

  std::size_t remainder = in_number % in_multiple;
  if (0 == remainder) {
    return in_number;
  }

  return in_number + in_multiple - remainder;
} // RoundUp

void* AllocateShaderMemory(uint32_t size) {
  #if defined(_WIN32) || defined(_WIN64)
    return reinterpret_cast<void*>(
      VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    );
  #else
    return mmap(
      NULL,
      size,
      PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_PRIVATE | MAP_ANONYMOUS,
      -1,
      0
    );
  #endif // _WIN32 || _WIN64
}

void FreeShaderMemory(void *ptr, uint32_t size) {
  #if defined(_WIN32) || defined(_WIN64)
    if (VirtualFree(ptr, size, MEM_DECOMMIT) == 0) {
      assert(false);
    }
    if (VirtualFree(ptr, 0, MEM_RELEASE) == 0) {
      assert(false);
    }
  #else
    if (munmap(ptr, size) != 0) {
      assert(false);
    }
  #endif // _WIN32 || _WIN64
}

struct CodeSymbol {
  Elf *elf;
  Elf64_Sym *sym;
  std::size_t strtabndx;
};

std::list<CodeSymbol> coss;

} // namespace anonymous

namespace core {
namespace loader {

//==============================================================================

hsa_status_t GetCodeObjectSymbolInfo(
  hsa_code_symbol_t code_symbol,
  hsa_code_symbol_info_t attribute,
  void *value
) {
  assert(value);

  CodeSymbol *cos  =reinterpret_cast<CodeSymbol *>(code_symbol.handle);
  Elf64_Sym *sym = cos->sym;
  if (!sym) {
    return HSA_STATUS_ERROR;
  }

  char *sn = NULL;
  if (NULL == (sn = elf_strptr(cos->elf, cos->strtabndx, sym->st_name)))
  {
    return HSA_STATUS_ERROR;
  }
  assert(sn);

  std::string tmp(sn);
  std::string acsn = tmp;
  std::string acmn = "";

  if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL) {
    size_t snndx = tmp.find(':') + 2;
    acmn = tmp.substr(0, snndx-2);
    acsn = tmp.substr(snndx);
  }

  switch (attribute) {
    case HSA_CODE_SYMBOL_INFO_TYPE: {
      if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC) {
        *((hsa_symbol_kind_t*)value) = HSA_SYMBOL_KIND_KERNEL;
      } else {
        *((hsa_symbol_kind_t*)value) = HSA_SYMBOL_KIND_VARIABLE;
      }
      break;
    }
    case HSA_CODE_SYMBOL_INFO_NAME_LENGTH: {
      *((uint32_t*)value) = static_cast<uint32_t>(strlen(sn));
      break;
    }
    case HSA_CODE_SYMBOL_INFO_NAME: {
      memcpy(value, sn, strlen(sn));
      break;
    }
    case HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH: {
      *((uint32_t*)value) = static_cast<uint32_t>(acmn.length());
      break;
    }
    case HSA_CODE_SYMBOL_INFO_MODULE_NAME: {
      memcpy(value, acmn.c_str(), acmn.length());
      break;
    }
    case HSA_CODE_SYMBOL_INFO_LINKAGE: {
      if (ELF64_ST_BIND(sym->st_info) == STB_GLOBAL) {
        *((hsa_symbol_linkage_t*)value) = HSA_SYMBOL_LINKAGE_PROGRAM;
      } else {
        *((hsa_symbol_linkage_t*)value) = HSA_SYMBOL_LINKAGE_MODULE;
      }
      break;
    }
    case HSA_CODE_SYMBOL_INFO_IS_DEFINITION: {
      UBits other;
      other.byte = sym->st_other;
      bool is_definition = other.bits.b7 == 1 ? true : false;
      *((bool*)value) = is_definition;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_ALLOCATION: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = sym->st_other;
      bool is_prog = other.bits.b6 == 1 ? true : false;
      *((hsa_variable_allocation_t*)value) = is_prog ?
        HSA_VARIABLE_ALLOCATION_PROGRAM : HSA_VARIABLE_ALLOCATION_AGENT;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_SEGMENT: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = sym->st_other;
      bool is_glob = other.bits.b5 == 1 ? true : false;
      *((hsa_variable_segment_t*)value) = is_glob?
        HSA_VARIABLE_SEGMENT_GLOBAL : HSA_VARIABLE_SEGMENT_READONLY;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT: {
      // TODO
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_SIZE: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      *((uint32_t*)value) = sym->st_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_IS_CONST: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = sym->st_other;
      bool is_const = other.bits.b3 == 1 ? true : false;
      *((bool*)value) = is_const;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      Elf_Scn *text = elf_getscn(cos->elf, sym->st_shndx);
      if (!text) {
        return HSA_STATUS_ERROR;
      }

      Elf_Data *text_data = NULL;
      text_data = elf_getdata(text, text_data);
      if (!text_data) {
        return HSA_STATUS_ERROR;
      }

      char *code = (char*)text_data->d_buf;
      code += sym->st_value;
      amd_kernel_code_t *akc = (amd_kernel_code_t*)code;

      // FIXME: uint32_t->uint64_t;
      *((uint32_t*)value) =
        static_cast<uint32_t>(akc->kernarg_segment_byte_size);
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      Elf_Scn *text = elf_getscn(cos->elf, sym->st_shndx);
      if (!text) {
        return HSA_STATUS_ERROR;
      }

      Elf_Data *text_data = NULL;
      text_data = elf_getdata(text, text_data);
      if (!text_data) {
        return HSA_STATUS_ERROR;
      }

      char *code = (char*)text_data->d_buf;
      code += sym->st_value;
      amd_kernel_code_t *akc = (amd_kernel_code_t*)code;

      // FIXME: uint32_t->uint8_t;
      *((uint32_t*)value) =
        static_cast<uint32_t>(akc->kernarg_segment_alignment);
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      Elf_Scn *text = elf_getscn(cos->elf, sym->st_shndx);
      if (!text) {
        return HSA_STATUS_ERROR;
      }

      Elf_Data *text_data = NULL;
      text_data = elf_getdata(text, text_data);
      if (!text_data) {
        return HSA_STATUS_ERROR;
      }

      char *code = (char*)text_data->d_buf;
      code += sym->st_value;
      amd_kernel_code_t *akc = (amd_kernel_code_t*)code;

      *((uint32_t*)value) = akc->workgroup_group_segment_byte_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      Elf_Scn *text = elf_getscn(cos->elf, sym->st_shndx);
      if (!text) {
        return HSA_STATUS_ERROR;
      }

      Elf_Data *text_data = NULL;
      text_data = elf_getdata(text, text_data);
      if (!text_data) {
        return HSA_STATUS_ERROR;
      }

      char *code = (char*)text_data->d_buf;
      code += sym->st_value;
      amd_kernel_code_t *akc = (amd_kernel_code_t*)code;

      *((uint32_t*)value) = akc->workitem_private_segment_byte_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK: {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      Elf_Scn *text = elf_getscn(cos->elf, sym->st_shndx);
      if (!text) {
        return HSA_STATUS_ERROR;
      }

      Elf_Data *text_data = NULL;
      text_data = elf_getdata(text, text_data);
      if (!text_data) {
        return HSA_STATUS_ERROR;
      }

      char *code = (char*)text_data->d_buf;
      code += sym->st_value;
      amd_kernel_code_t *akc = (amd_kernel_code_t*)code;

      *((bool*)value) = akc->is_dynamic_callstack;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION: {
      // TODO
      break;
    }
    default: {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t GetCodeObjectSymbol(
  hsa_code_object_t code_object,
  const char *symbol_name,
  hsa_code_symbol_t *symbol
) {
  assert(symbol_name);
  assert(symbol);

  if (EV_NONE == elf_version(EV_CURRENT)) {
    return HSA_STATUS_ERROR;
  }

  CodeObject *mco = reinterpret_cast<CodeObject*>(code_object.handle);
  if (!mco) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  assert(mco->elf);
  assert(mco->ed);

  size_t estsi = 0;
  int eec = elf_getshdrstrndx(mco->ed, &estsi);
  if (0 != eec) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  Elf64_Sym *symbol_table = NULL;
  std::size_t symbol_table_size = 0;
  std::size_t strtabndx = 0;

  Elf_Scn *escn = NULL;
  while (NULL != (escn = elf_nextscn(mco->ed, escn))) {
    GElf_Shdr eshdr;
    if (&eshdr != gelf_getshdr(escn, &eshdr)) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    char *esn = NULL;
    if (NULL == (esn = elf_strptr(mco->ed, estsi, eshdr.sh_name))) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    switch (eshdr.sh_type) {
      case SHT_SYMTAB: {
        if (0 == strncmp(esn, ".symtab", 7)) {
          Elf_Data *symtab_section = NULL;
          symtab_section = elf_getdata(escn, symtab_section);
          if (!symtab_section) {
            return HSA_STATUS_ERROR;
          }
          symbol_table = (Elf64_Sym*)symtab_section->d_buf;
          symbol_table_size = symtab_section->d_size;
          strtabndx = eshdr.sh_link;
        }
          break;
        }
      default: {
        break;
      }
    }
  }

  assert(symbol_table);
  assert(symbol_table_size > 0);


  for (size_t i = 0; i < symbol_table_size / sizeof(Elf64_Sym); ++i) {
    char *sn = NULL;
    if (NULL == (sn = elf_strptr(mco->ed, strtabndx, symbol_table[i].st_name)))
    {
      return HSA_STATUS_ERROR;
    }
    assert(sn);

    size_t sm = std::max(strlen(sn), strlen(symbol_name));

    if (strncmp(sn, symbol_name, sm) == 0) {
      coss.push_back(CodeSymbol());
      coss.back().elf = mco->ed;
      coss.back().sym = symbol_table + i;
      coss.back().strtabndx = strtabndx;

      symbol->handle = reinterpret_cast<uint64_t>(&coss.back());
      return HSA_STATUS_SUCCESS;
    }
  }

  return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
}

hsa_status_t GetCodeObjectInfo(
  hsa_code_object_t code_object,
  hsa_code_object_info_t attribute,
  void *value
) {
  assert(value);

  if (EV_NONE == elf_version(EV_CURRENT)) {
    return HSA_STATUS_ERROR;
  }

  CodeObject *mco = reinterpret_cast<CodeObject*>(code_object.handle);
  if (!mco) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  assert(mco->elf);
  assert(mco->ed);

  size_t estsi = 0;
  int eec = elf_getshdrstrndx(mco->ed, &estsi);
  if (0 != eec) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  Elf_Scn *escn = NULL;
  while (NULL != (escn = elf_nextscn(mco->ed, escn))) {
    GElf_Shdr eshdr;
    if (&eshdr != gelf_getshdr(escn, &eshdr)) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    char *esn = NULL;
    if (NULL == (esn = elf_strptr(mco->ed, estsi, eshdr.sh_name))) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    switch (eshdr.sh_type) {
      case SHT_NOTE: {
        if (0 == strncmp(esn, ".note", 5)) {
          Elf_Data *note_section = NULL;
          note_section = elf_getdata(escn, note_section);
          if (!note_section) {
            return HSA_STATUS_ERROR;
          }

          uint32_t looking_for = 0;
          switch (attribute) {
            case HSA_CODE_OBJECT_INFO_VERSION: {
              looking_for = 124;
              break;
            }
            case HSA_CODE_OBJECT_INFO_TYPE: {
              *((hsa_code_object_type_t*)value) = HSA_CODE_OBJECT_TYPE_PROGRAM;
              return HSA_STATUS_SUCCESS;
            }
            case HSA_CODE_OBJECT_INFO_ISA: {
              looking_for = 128;
              break;
            }
            case HSA_CODE_OBJECT_INFO_MACHINE_MODEL: {
              looking_for = 127;
              break;
            }
            case HSA_CODE_OBJECT_INFO_PROFILE: {
              looking_for = 126;
              break;
            }
            case HSA_CODE_OBJECT_INFO_DEFAULT_FLOAT_ROUNDING_MODE: {
              looking_for = 125;
              break;
            }
            default: {
              assert(false);
            }
          }

          uint32_t *notes = (uint32_t*)note_section->d_buf;
          for (size_t i = 0; i < note_section->d_size / 4; ++i) {
            if (notes[i] == looking_for) {
              assert(i >= 2);
              uint32_t namesz = notes[i-2];
              uint32_t namesz_rounded =
                RoundUp(static_cast<size_t>(namesz), 4) / 4;
              char *data = (char*)note_section->d_buf;
              data += (i+1) * 4 + namesz_rounded * 4;
              size_t data_length = strlen(data);

              switch(looking_for) {
                case 124: {
                  memcpy(value, data, data_length);
                  break;
                }
                case 125: {
                  if (strncmp(data, "ZERO", 4) == 0) {
                    *((hsa_default_float_rounding_mode_t*)value) =
                      HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO;
                  } else {
                    *((hsa_default_float_rounding_mode_t*)value) =
                      HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR;
                  }
                  break;
                }
                case 126: {
                  if (strncmp(data, "FULL", 4) == 0) {
                    *((hsa_profile_t*)value) = HSA_PROFILE_FULL;
                  } else {
                    *((hsa_profile_t*)value) = HSA_PROFILE_BASE;
                  }
                  break;
                }
                case 127: {
                  if (strncmp(data, "SMALL", 4) == 0) {
                    *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_SMALL;
                  } else {
                    *((hsa_machine_model_t*)value) = HSA_MACHINE_MODEL_LARGE;
                  }
                  break;
                }
                case 128: {
                  return
                  core::loader::Isa::Create("AMD:GPU:7:0:0", (hsa_isa_t*)value);
                  break;
                }
              }

            }
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t SerializeCodeObject(
  hsa_code_object_t code_object,
  hsa_status_t (*alloc_callback)(
    size_t size, hsa_callback_data_t data, void **address
  ),
  hsa_callback_data_t callback_data,
  const char *options,
  void **serialized_code_object,
  size_t *serialized_code_object_size
) {
  assert(alloc_callback);
  assert(serialized_code_object);
  assert(serialized_code_object_size);

  CodeObject *mco = reinterpret_cast<CodeObject*>(code_object.handle);
  if (!mco) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  assert(mco->elf);
  assert(mco->ed);

  if (mco->globals) {
    return HSA_STATUS_ERROR;
  }

  hsa_status_t hsc =
    alloc_callback(mco->size, callback_data, serialized_code_object);
  if (HSA_STATUS_SUCCESS != hsc) {
    return hsc;
  }

  memcpy(*serialized_code_object, mco->elf, mco->size);
  *serialized_code_object_size = mco->size;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t DeserializeCodeObject(
  void *serialized_code_object,
  size_t serialized_code_object_size,
  const char *options,
  hsa_code_object_t *code_object
) {
  assert(serialized_code_object);
  assert(serialized_code_object_size > 0);
  assert(code_object);

  void *co = malloc(serialized_code_object_size);
  if (!co) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  memcpy(co, serialized_code_object, serialized_code_object_size);

  if (EV_NONE == elf_version(EV_CURRENT)) {
    return HSA_STATUS_ERROR;
  }

  Elf *elf = elf_memory((char*)co, serialized_code_object_size);
  if (!elf) {
    free(co);
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  CodeObject *coo = new CodeObject;
  if (!coo) {
    elf_end(elf);
    free(co);
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }
  coo->elf = co;
  coo->size = serialized_code_object_size;
  coo->globals = false;
  coo->loaded = false;
  coo->ed = elf;

  code_object->handle = reinterpret_cast<uint64_t>(coo);
  return HSA_STATUS_SUCCESS;
}

//==============================================================================


hsa_status_t DestroyCodeObject(hsa_code_object_t code_object) {
  CodeObject *mco = reinterpret_cast<CodeObject*>(code_object.handle);
  if (!mco) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (mco->loaded) {
    return HSA_STATUS_ERROR;
  }

  assert(mco->elf);
  assert(mco->ed);

  if (mco->globals) {
    size_t estsi = 0;
    int eec = elf_getshdrstrndx(mco->ed, &estsi);
    if (0 != eec) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    Elf_Scn *escn = NULL;
    while (NULL != (escn = elf_nextscn(mco->ed, escn))) {
      GElf_Shdr eshdr;
      if (&eshdr != gelf_getshdr(escn, &eshdr)) {
        // \todo new error code?
        return HSA_STATUS_ERROR;
      }

      char *esn = NULL;
      if (NULL == (esn = elf_strptr(mco->ed, estsi, eshdr.sh_name))) {
        // \todo new error code?
        return HSA_STATUS_ERROR;
      }

      switch (eshdr.sh_type) {
        case SHT_NOBITS: {
          if (0 == strncmp(esn, ".data.agent", 11)) {
            void *base = reinterpret_cast<void*>(eshdr.sh_addr);
            HSA::hsa_memory_free(base);
          }
          if (0 == strncmp(esn, ".data.prog", 10)) {
            void *base = reinterpret_cast<void*>(eshdr.sh_addr);
            HSA::hsa_memory_free(base);
          }
          break;
        }
        default: {
          break;
        }
      }
    }
  }

  elf_end(mco->ed);
  free(mco->elf);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t GetSymbolInfo(
  hsa_executable_symbol_t executable_symbol,
  hsa_executable_symbol_info_t attribute,
  void *value
) {

  std::pair<Elf64_Sym, SymbolInfo> *symbol =
    reinterpret_cast<std::pair<Elf64_Sym, SymbolInfo>*>(
      executable_symbol.handle);
  if (!symbol) {
    return HSA_STATUS_ERROR;
  }

  if (!value) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  char *symbol_name = NULL;
  if (NULL == (symbol_name = elf_strptr(symbol->second.elf,
                                        symbol->second.sstrtabndx,
                                        symbol->first.st_name))) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  std::string tmp(symbol_name);
  std::string acsn = tmp;
  std::string acmn = "";

  if (ELF64_ST_BIND(symbol->first.st_info) != STB_GLOBAL) {
    size_t snndx = tmp.find(':') + 2;
    acmn = tmp.substr(0, snndx-2);
    acsn = tmp.substr(snndx);
  }

  switch (attribute) {
    case HSA_EXECUTABLE_SYMBOL_INFO_TYPE: {
      if (ELF64_ST_TYPE(symbol->first.st_info) == STT_FUNC) {
        *((hsa_symbol_kind_t*)value) = HSA_SYMBOL_KIND_KERNEL;
      } else {
        *((hsa_symbol_kind_t*)value) = HSA_SYMBOL_KIND_VARIABLE;
      }
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH: {
      *((uint32_t*)value) = static_cast<uint32_t>(acsn.length());
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME: {
      memmove(value, acsn.c_str(), acsn.length());
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH: {
      *((uint32_t*)value) = static_cast<uint32_t>(acmn.length());
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME: {
      memmove(value, acmn.c_str(), acmn.length());
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_AGENT: {
      *((hsa_agent_t*)value) = symbol->second.agent;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      *((uint64_t*)value) = symbol->second.address;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_LINKAGE: {
      if (ELF64_ST_BIND(symbol->first.st_info) == STB_GLOBAL) {
        *((hsa_symbol_linkage_t*)value) = HSA_SYMBOL_LINKAGE_PROGRAM;
      } else {
        *((hsa_symbol_linkage_t*)value) = HSA_SYMBOL_LINKAGE_MODULE;
      }
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_IS_DEFINITION: {
      UBits other;
      other.byte = symbol->first.st_other;
      bool is_definition = other.bits.b7 == 1 ? true : false;
      *((bool*)value) = is_definition;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALLOCATION: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = symbol->first.st_other;
      bool is_prog = other.bits.b6 == 1 ? true : false;
      *((hsa_variable_allocation_t*)value) = is_prog ?
        HSA_VARIABLE_ALLOCATION_PROGRAM : HSA_VARIABLE_ALLOCATION_AGENT;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SEGMENT: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = symbol->first.st_other;
      bool is_glob = other.bits.b5 == 1 ? true : false;
      *((hsa_variable_segment_t*)value) = is_glob?
        HSA_VARIABLE_SEGMENT_GLOBAL : HSA_VARIABLE_SEGMENT_READONLY;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT: {
      // TODO
      assert(false);
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      *((uint32_t*)value) = symbol->first.st_size;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_IS_CONST: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_OBJECT) {
        return HSA_STATUS_ERROR;
      }
      UBits other;
      other.byte = symbol->first.st_other;
      bool is_const = other.bits.b3 == 1 ? true : false;
      *((bool*)value) = is_const;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      *((uint64_t*)value) = symbol->second.address;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      amd_kernel_code_t *akc = (amd_kernel_code_t*)symbol->second.address;
      *((uint32_t*)value) =
        static_cast<uint32_t>(akc->kernarg_segment_byte_size);
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
    }
      amd_kernel_code_t *akc = (amd_kernel_code_t*)symbol->second.address;
      *((uint32_t*)value) =
        static_cast<uint32_t>(akc->kernarg_segment_alignment);
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      amd_kernel_code_t *akc = (amd_kernel_code_t*)symbol->second.address;
      *((uint32_t*)value) = akc->workgroup_group_segment_byte_size;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      amd_kernel_code_t *akc = (amd_kernel_code_t*)symbol->second.address;
      *((uint32_t*)value) = akc->workitem_private_segment_byte_size;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK: {
      if (ELF64_ST_TYPE(symbol->first.st_info) != STT_FUNC) {
        return HSA_STATUS_ERROR;
      }
      amd_kernel_code_t *akc = (amd_kernel_code_t*)symbol->second.address;
      *((bool*)value) = akc->is_dynamic_callstack;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_OBJECT: {
      // TODO
      assert(false);
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION: {
      // TODO
      assert(false);
      break;
    }
    default: {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::Create(
  hsa_profile_t profile,
  hsa_executable_state_t executable_state,
  const char *options,
  hsa_executable_t *executable
) {
  if (HSA_PROFILE_BASE != profile && HSA_PROFILE_FULL != profile) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  if (!executable) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  Executable *exec = new (std::nothrow) Executable(
    profile, executable_state
  );
  if (!exec) { return HSA_STATUS_ERROR_OUT_OF_RESOURCES; }

  *executable = Executable::Handle(exec);
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::Destroy(hsa_executable_t executable) {
  Executable *exec = reinterpret_cast<Executable*>(executable.handle);
  if (!exec) { return HSA_STATUS_ERROR_INVALID_EXECUTABLE; }

  HSA::hsa_memory_deregister(exec->code_base_, 0);
  FreeShaderMemory(exec->code_base_, exec->code_size_);

  exec->execs_.clear();

  return HSA_STATUS_SUCCESS;
}

hsa_executable_t Executable::Handle(const Executable *exec) {
  hsa_executable_t executable;
  executable.handle = reinterpret_cast<uint64_t>(exec);
  return executable;
}

Executable* Executable::Object(const hsa_executable_t &executable) {
  return reinterpret_cast<Executable*>(executable.handle);
}

hsa_status_t Executable::Freeze(const char *options) {
  if (this->executable_state_ == HSA_EXECUTABLE_STATE_FROZEN) {
    return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
  }
  this->executable_state_ = HSA_EXECUTABLE_STATE_FROZEN;
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::GetInfo(
  const hsa_executable_info_t &attribute, void *value
) {
  if (!value) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  switch (attribute) {
    case HSA_EXECUTABLE_INFO_PROFILE: {
      *((hsa_profile_t*)value) = profile_;
      return HSA_STATUS_SUCCESS;
    }
    case HSA_EXECUTABLE_INFO_STATE: {
      *((hsa_executable_state_t*)value) = executable_state_;
      return HSA_STATUS_SUCCESS;
    }
    default: {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }
}

hsa_status_t Executable::GetSymbol(
  const char *module_name,
  const char *symbol_name,
  hsa_agent_t agent,
  int32_t call_convention,
  hsa_executable_symbol_t *symbol
) {
  if (!symbol_name || !symbol) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  auto fs = execs_.find(std::string(symbol_name));
  if (fs == execs_.end()) {
    return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
  }

  symbol->handle = reinterpret_cast<uint64_t>(&(fs->second));
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::LoadCodeObject(
  hsa_agent_t agent,
  hsa_code_object_t code_object,
  const char *options
) {
  if (executable_state_ == HSA_EXECUTABLE_STATE_FROZEN) {
    return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
  }

  CodeObject *mco = reinterpret_cast<CodeObject*>(code_object.handle);
  if (!mco) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (mco->loaded) {
    return HSA_STATUS_ERROR;
  }

  assert(mco->elf);
  assert(mco->ed);

  size_t estsi = 0;
  int eec = elf_getshdrstrndx(mco->ed, &estsi);
  if (0 != eec) {
    // \todo new error code?
    return HSA_STATUS_ERROR;
  }

  Elf_Data *code = NULL;
  Elf_Data *symtab = NULL;
  std::size_t strtabndx = 0;

  Elf_Scn *escn = NULL;
  while (NULL != (escn = elf_nextscn(mco->ed, escn))) {
    GElf_Shdr eshdr;
    if (&eshdr != gelf_getshdr(escn, &eshdr)) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    char *esn = NULL;
    if (NULL == (esn = elf_strptr(mco->ed, estsi, eshdr.sh_name))) {
      // \todo new error code?
      return HSA_STATUS_ERROR;
    }

    switch (eshdr.sh_type) {
      case SHT_NOBITS: {
          if (0 == strncmp(esn, ".data.agent", 11)) {
            agent_base_ = static_cast<uint64_t>(eshdr.sh_addr);
          }
          if (0 == strncmp(esn, ".data.prog", 10)) {
            prog_base_ = static_cast<uint64_t>(eshdr.sh_addr);
          }
          break;
      }
      case SHT_SYMTAB: {
        if (0 == strncmp(esn, ".symtab", 7)) {
          symtab = elf_getdata(escn, symtab);
          if (!symtab) {
            return HSA_STATUS_ERROR;
          }
          strtabndx = eshdr.sh_link;
        }
        break;
      }
      case SHT_PROGBITS: {
        if (0 == strncmp(esn, ".text", 5)) {
          code = elf_getdata(escn, code);
          if (!code) {
            return HSA_STATUS_ERROR;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  assert(symtab);
  assert(code);

  code_base_ = (char*)AllocateShaderMemory(code->d_size);
  if (!code_base_) {
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  HSA::hsa_memory_register(code_base_, code->d_size);

  code_size_ = code->d_size;
  memcpy(code_base_, code->d_buf, code->d_size);

  Elf64_Sym *symbols = (Elf64_Sym*)symtab->d_buf;
  for (size_t i = 0; i < symtab->d_size / sizeof(Elf64_Sym); ++i) {
    char *sn = NULL;
    if (NULL == (sn = elf_strptr(mco->ed, strtabndx, symbols[i].st_name)))
    {
      return HSA_STATUS_ERROR;
    }
    assert(sn);

    UBits other;
    other.byte = symbols[i].st_other;

    uint64_t address = 0;

    if (other.bits.b7 == 1) {
      if (ELF64_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
        address = reinterpret_cast<uint64_t>(code_base_);
        kernels_.push_back(KernelSymbol(mco->elf, mco->size, i));
        amd_kernel_code_t *akc =
          reinterpret_cast<amd_kernel_code_t*>(address + symbols[i].st_value);
        akc->runtime_loader_kernel_symbol =
          reinterpret_cast<uint64_t>(&kernels_.back());
      } else {
        if (other.bits.b6 == 1) {
          address = prog_base_;
        } else {
          address = agent_base_;
        }
      }
    }

    std::pair<Elf64_Sym, SymbolInfo> p = std::make_pair(
      symbols[i],
      SymbolInfo(symbols[i].st_value + address, mco->ed, agent, strtabndx)
    );

    execs_.insert(std::make_pair(std::string(sn),p));
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::IterateSymbols(
  hsa_status_t (*callback)(
    hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data
  ),
  void *data
) {
  assert(callback);

  for (auto &symbol_entry : execs_) {
    hsa_executable_symbol_t exec_symbol;
    exec_symbol.handle = reinterpret_cast<uint64_t>(&(symbol_entry.second));
    hsa_status_t hsc = callback(Handle(this), exec_symbol, data);
    if (HSA_STATUS_SUCCESS != hsc) {
      return hsc;
    }
  }

  return HSA_STATUS_SUCCESS;
}

} // namespace loader
} // namespace core
