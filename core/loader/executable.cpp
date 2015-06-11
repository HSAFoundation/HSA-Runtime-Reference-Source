#include "core/loader/executable.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <libelf.h>
#include "hsa_ext_amd.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif // _WIN32 || _WIN64

namespace {

void* AllocateCodeMemory(const size_t &size) {
  void *code_mem = NULL;

  #if defined(_WIN32) || defined(_WIN64)
    code_mem = reinterpret_cast<void*>(
      VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
  #else
    code_mem = mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  #endif // _WIN32 || _WIN64

  assert(code_mem);
  hsa_memory_register(code_mem, size);

  return code_mem;
}

void DeallocateCodeMemory(void *ptr, const size_t &size) {
  assert(ptr);
  hsa_memory_deregister(ptr, 0);

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

hsa_status_t FindRegion(hsa_region_segment_t seg, hsa_region_t reg, void *data) {
  if (!data) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  bool is_host_region;
  hsa_status_t hsc = hsa_region_get_info(
    reg, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &is_host_region);
  if (HSA_STATUS_SUCCESS != hsc) {
    return hsc;
  }

  if (is_host_region) {
    hsa_region_segment_t segment;
    hsc = hsa_region_get_info(
      reg, HSA_REGION_INFO_SEGMENT, &segment);
    if (HSA_STATUS_SUCCESS != hsc) {
      return hsc;
    }

    if (segment == seg) {
      *((hsa_region_t*)data) = reg;
    }
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t FindGlobalRegion(hsa_region_t reg, void *data) {
  return FindRegion(HSA_REGION_SEGMENT_GLOBAL, reg, data);
}

hsa_status_t FindReadonlyRegion(hsa_region_t reg, void *data) {
  return FindRegion(HSA_REGION_SEGMENT_READONLY, reg, data);
}

size_t RoundUp(const std::size_t &in_number,
               const std::size_t &in_multiple) {
  if (0 == in_multiple) {
    return in_number;
  }

  std::size_t remainder = in_number % in_multiple;
  if (0 == remainder) {
    return in_number;
  }

  return in_number + in_multiple - remainder;
}

//===----------------------------------------------------------------------===//
// TODO(kzhuravl): this is re-defined, remove when relocating loader.

// Elf header values.
#define EM_AMDGPU 224
#define ELFOSABI_AMDGPU_HSA 64
#define ELFABIVERSION_AMGDGPU_HSA_CURRENT 0

// Program header types.
#define PT_AMDGPU_HSA_LOAD_GLOBAL_PROGRAM (PT_LOOS)
#define PT_AMDGPU_HSA_LOAD_GLOBAL_AGENT   (PT_LOOS + 1)
#define PT_AMDGPU_HSA_LOAD_READONLY_AGENT (PT_LOOS + 2)
#define PT_AMDGPU_HSA_LOAD_CODE_AGENT     (PT_LOOS + 3)

// Section flags.
#define SHF_AMDGPU_HSA_GLOBAL   (0x00100000 & SHF_MASKOS)
#define SHF_AMDGPU_HSA_READONLY (0x00200000 & SHF_MASKOS)
#define SHF_AMDGPU_HSA_CODE     (0x00400000 & SHF_MASKOS)
#define SHF_AMDGPU_HSA_AGENT    (0x00800000 & SHF_MASKOS)

// Symbol types.
#define STT_AMDGPU_HSA_KERNEL            (STT_LOOS)
#define STT_AMDGPU_HSA_INDIRECT_FUNCTION (STT_LOOS + 1)
#define STT_AMDGPU_HSA_METADATA          (STT_LOOS + 2)

// Relocation types.
#define R_AMDGPU_NONE         0
#define R_AMDGPU_32_LOW       1
#define R_AMDGPU_32_HIGH      2
#define R_AMDGPU_64           3
#define R_AMDGPU_INIT_SAMPLER 4
#define R_AMDGPU_INIT_IMAGE   5

// Note types.
#define NT_AMDGPU_HSA_CODE_OBJECT_VERSION 1
#define NT_AMDGPU_HSA_HSAIL               2
#define NT_AMDGPU_HSA_ISA                 3
#define NT_AMDGPU_HSA_PRODUCER            4
#define NT_AMDGPU_HSA_PRODUCER_OPTIONS    5

// Image metadata types.
typedef uint16_t amdgpu_hsa_metadata_kind16_t;
enum hco_metadata_t {
  AMDGPU_HSA_METADATA_KIND_NONE = 0,
  AMDGPU_HSA_METADATA_KIND_INIT_SAMP = 1,
  AMDGPU_HSA_METADATA_KIND_INIT_ROIMG = 2,
  AMDGPU_HSA_METADATA_KIND_INIT_WOIMG = 3,
  AMDGPU_HSA_METADATA_KIND_INIT_RWIMG = 4
};

// Sampler descriptor.
typedef struct amdgpu_hsa_sampler_descriptor_s {
  uint16_t size;
  amdgpu_hsa_metadata_kind16_t kind;
  uint8_t coord;
  uint8_t filter;
  uint8_t addressing;
  uint8_t reserved;
} amdgpu_hsa_sampler_descriptor_t;

// Image descriptor.
typedef struct amdgpu_hsa_image_descriptor_s {
  uint16_t size;
  amdgpu_hsa_metadata_kind16_t kind;
  uint8_t geometry;
  uint8_t channel_order;
  uint8_t channel_type;
  uint8_t reserved;
  uint64_t width;
  uint64_t height;
  uint64_t depth;
  uint64_t array;
} amdgpu_hsa_image_descriptor_t;

// amd_kernel_code_t.
typedef struct amd_kernel_code_s {
  uint32_t amd_kernel_code_version_major;
  uint32_t amd_kernel_code_version_minor;
  uint16_t amd_machine_kind;
  uint16_t amd_machine_version_major;
  uint16_t amd_machine_version_minor;
  uint16_t amd_machine_version_stepping;
  int64_t kernel_code_entry_byte_offset;
  int64_t kernel_code_prefetch_byte_offset;
  uint64_t kernel_code_prefetch_byte_size;
  uint64_t max_scratch_backing_memory_byte_size;
  uint64_t compute_pgm_resource_registers;
  uint32_t enable_sgpr_private_segment_buffer :1;
  uint32_t enable_sgpr_dispatch_ptr :1;
  uint32_t enable_sgpr_queue_ptr :1;
  uint32_t enable_sgpr_kernarg_segment_ptr :1;
  uint32_t enable_sgpr_dispatch_id :1;
  uint32_t enable_sgpr_flat_scratch_init :1;
  uint32_t enable_sgpr_private_segment_size :1;
  uint32_t enable_sgpr_grid_workgroup_count_x :1;
  uint32_t enable_sgpr_grid_workgroup_count_y :1;
  uint32_t enable_sgpr_grid_workgroup_count_z :1;
  uint32_t reserved1 :6;
  uint32_t enable_ordered_append_gds :1;
  uint32_t private_element_size :2;
  uint32_t is_ptr64 :1;
  uint32_t is_dynamic_callstack :1;
  uint32_t is_debug_enabled :1;
  uint32_t is_xnack_enabled :1;
  uint32_t reserved2 :9;
  uint32_t workitem_private_segment_byte_size;
  uint32_t workgroup_group_segment_byte_size;
  uint32_t gds_segment_byte_size;
  uint64_t kernarg_segment_byte_size;
  uint32_t workgroup_fbarrier_count;
  uint16_t wavefront_sgpr_count;
  uint16_t workitem_vgpr_count;
  uint16_t reserved_vgpr_first;
  uint16_t reserved_vgpr_count;
  uint16_t reserved_sgpr_first;
  uint16_t reserved_sgpr_count;
  uint16_t debug_wavefront_private_segment_offset_sgpr;
  uint16_t debug_private_segment_buffer_sgpr;
  uint8_t kernarg_segment_alignment;
  uint8_t group_segment_alignment;
  uint8_t private_segment_alignment;
  uint8_t wavefront_size;
  int32_t call_convention;
  uint8_t reserved3[12];
  uint64_t runtime_loader_kernel_symbol;
  uint64_t control_directives[16];
} amd_kernel_code_t;

static_assert(256 == sizeof(amd_kernel_code_t), "invalid amd_kernel_code_t size");

//===----------------------------------------------------------------------===//

} // namespace anonymous

namespace core {
namespace loader {

//===----------------------------------------------------------------------===//
// ElfMemoryImage - Templates.                                                //
//===----------------------------------------------------------------------===//

template<typename ElfN_Ehdr, typename ElfN_Shdr>
uint16_t ElfMemoryImage::FindSNDX(const void *emi,
                                  const uint32_t &type,
                                  const uint64_t &flags) {
  assert(IsValid(emi));

  const ElfN_Ehdr *ehdr = (const ElfN_Ehdr*)emi;
  if (NULL == ehdr || EV_CURRENT != ehdr->e_version) {
    return false;
  }

  const ElfN_Shdr *shdr = (const ElfN_Shdr*)((char*)emi + ehdr->e_shoff);
  if (NULL == shdr) {
    return false;
  }

  for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
    if (shdr[i].sh_type == type) {
      if (flags && !(shdr[i].sh_flags & flags)) {
        continue;
      }
      return i;
    }
  }

  return 0;
}

template<typename ElfN_Ehdr>
bool ElfMemoryImage::IsAmdGpu(const void *emi) {
  assert(IsValid(emi));

  const ElfN_Ehdr *ehdr = (const ElfN_Ehdr*)emi;
  if (NULL == ehdr || EV_CURRENT != ehdr->e_version) {
    return false;
  }

  return EM_AMDGPU == ehdr->e_machine;
}

template<typename ElfN_Ehdr, typename ElfN_Shdr>
uint64_t ElfMemoryImage::Size(const void *emi) {
  assert(IsValid(emi));

  const ElfN_Ehdr *ehdr = (const ElfN_Ehdr*)emi;
  if (NULL == ehdr || EV_CURRENT != ehdr->e_version) {
    return false;
  }

  const ElfN_Shdr *shdr = (const ElfN_Shdr*)((char*)emi + ehdr->e_shoff);
  if (NULL == shdr) {
    return false;
  }

  uint64_t max_offset = ehdr->e_shoff;
  uint64_t total_size = max_offset + ehdr->e_shentsize * ehdr->e_shnum;

  for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
    uint64_t cur_offset = static_cast<uint64_t>(shdr[i].sh_offset);
    if (max_offset < cur_offset) {
      max_offset = cur_offset;
      total_size = max_offset;
      if (SHT_NOBITS != shdr[i].sh_type) {
        total_size += static_cast<uint64_t>(shdr[i].sh_size);
      }
    }
  }

  return total_size;
}

//===----------------------------------------------------------------------===//
// ElfMemoryImage.                                                            //
//===----------------------------------------------------------------------===//

uint16_t ElfMemoryImage::FindSNDX(const void *emi,
                                  const uint32_t &type,
                                  const uint64_t &flags) {
  if (Is32(emi)) {
    return FindSNDX<Elf32_Ehdr, Elf32_Shdr>(emi, type, flags);
  } else if (Is64(emi)) {
    return FindSNDX<Elf64_Ehdr, Elf64_Shdr>(emi, type, flags);
  }
  return 0;
}

bool ElfMemoryImage::Is32(const void *emi) {
  if (!IsValid(emi)) {
    return false;
  }

  const unsigned char *e_ident = (const unsigned char*)emi;
  assert(NULL != e_ident);
  return ELFCLASS32 == e_ident[EI_CLASS];
}

bool ElfMemoryImage::Is64(const void *emi) {
  if (!IsValid(emi)) {
    return false;
  }

  const unsigned char *e_ident = (const unsigned char*)emi;
  assert(NULL != e_ident);
  return ELFCLASS64 == e_ident[EI_CLASS];
}

bool ElfMemoryImage::IsAmdGpu(const void *emi) {
  if (Is32(emi)) {
    return IsAmdGpu<Elf32_Ehdr>(emi);
  } else if (Is64(emi)) {
    return IsAmdGpu<Elf64_Ehdr>(emi);
  }
  return false;
}

bool ElfMemoryImage::IsBigEndian(const void *emi) {
  if (!IsValid(emi)) {
    return false;
  }

  const unsigned char *e_ident = (const unsigned char*)emi;
  assert(NULL != e_ident);
  return ELFDATA2MSB == e_ident[EI_DATA];
}

bool ElfMemoryImage::IsLittleEndian(const void *emi) {
  if (!IsValid(emi)) {
    return false;
  }

  const unsigned char *e_ident = (const unsigned char*)emi;
  assert(NULL != e_ident);
  return ELFDATA2LSB == e_ident[EI_DATA];
}

bool ElfMemoryImage::IsValid(const void *emi) {
  if (NULL == emi) {
    return false;
  }

  const unsigned char *e_ident = (const unsigned char*)emi;
  if (NULL == e_ident) {
    return false;
  }

  if (ELFMAG0 != e_ident[EI_MAG0]) {
    return false;
  }
  if (ELFMAG1 != e_ident[EI_MAG1]) {
    return false;
  }
  if (ELFMAG2 != e_ident[EI_MAG2]) {
    return false;
  }
  if (ELFMAG3 != e_ident[EI_MAG3]) {
    return false;
  }

  return true;
}

uint64_t ElfMemoryImage::Size(const void *emi) {
  if (Is32(emi)) {
    return Size<Elf32_Ehdr, Elf32_Shdr>(emi);
  } else if (Is64(emi)) {
    return Size<Elf64_Ehdr, Elf64_Shdr>(emi);
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// AmdGpuImage.                                                               //
//===----------------------------------------------------------------------===//

void AmdGpuImage::DestroySymbols(void *amdgpumi) {
  assert(ElfMemoryImage::Is64(amdgpumi));
  assert(ElfMemoryImage::IsAmdGpu(amdgpumi));

  auto symbol_entry = symbols_.find(amdgpumi);
  if (symbol_entry == symbols_.end()) {
    return;
  }
  if (!symbol_entry->second) {
    return;
  }

  for (size_t i = 0; i < symbol_entry->second->size(); ++i) {
    if (symbol_entry->second->at(i)) {
      delete symbol_entry->second->at(i);
    }
  }
}

hsa_status_t AmdGpuImage::GetInfo(void *amdgpumi,
                                  hsa_code_object_info_t attribute,
                                  void *value) {
  assert(ElfMemoryImage::Is64(amdgpumi));
  assert(ElfMemoryImage::IsAmdGpu(amdgpumi));

  assert(value);

  uint8_t hsa2elfATTR = 0;
  switch (attribute) {
    case HSA_CODE_OBJECT_INFO_VERSION: {
      hsa2elfATTR = NT_AMDGPU_HSA_CODE_OBJECT_VERSION;
      break;
    }
    case HSA_CODE_OBJECT_INFO_TYPE: {
      *((hsa_code_object_type_t*)value) = HSA_CODE_OBJECT_TYPE_PROGRAM;
      break;
    }
    case HSA_CODE_OBJECT_INFO_ISA: {
      hsa2elfATTR = NT_AMDGPU_HSA_ISA;
      break;
    }
    case HSA_CODE_OBJECT_INFO_MACHINE_MODEL:
    case HSA_CODE_OBJECT_INFO_PROFILE:
    case HSA_CODE_OBJECT_INFO_DEFAULT_FLOAT_ROUNDING_MODE: {
      hsa2elfATTR = NT_AMDGPU_HSA_HSAIL;
      break;
    }
    default: {
      assert(false);
    }
  }
  assert(hsa2elfATTR);

  uint64_t amdgpumisz = ElfMemoryImage::Size(amdgpumi);
  if (!amdgpumisz) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  uint16_t noteSNDX = ElfMemoryImage::FindSNDX(amdgpumi, SHT_NOTE);
  if (!noteSNDX) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  Elf *amdgpuemi = NULL;
  amdgpuemi = elf_memory((char*)amdgpumi, size_t(amdgpumisz));
  assert(amdgpuemi);

  Elf_Scn *noteSCN = NULL;
  noteSCN = elf_getscn(amdgpuemi, size_t(noteSNDX));
  assert(noteSCN);

  Elf_Data *noteDAT = NULL;
  noteDAT = elf_getdata(noteSCN, noteDAT);
  assert(noteDAT);

  uint32_t *rawDAT = (uint32_t*)noteDAT->d_buf;
  assert(rawDAT);
  size_t rawSZ = noteDAT->d_size;
  assert(rawSZ);
  assert(!(rawSZ % 4));

  uint32_t *rawNOTE = NULL;
  for (size_t i = 0; i < rawSZ / 4;) {
    i += 2;

    if (rawDAT[i] == hsa2elfATTR) {
      assert(1 < i);
      rawNOTE = rawDAT + i;
      break;
    }

    size_t newI = 0;
    newI += RoundUp(size_t(*(rawDAT + i - 2)), 4) / 4;
    newI += RoundUp(size_t(*(rawDAT + i - 1)), 4) / 4;
    i += newI + 1;
  }
  assert(rawNOTE);

  switch (attribute) {
    case HSA_CODE_OBJECT_INFO_VERSION: {
      NDVersion *ndVER =
        (NDVersion*)(rawNOTE + RoundUp(size_t(*(rawNOTE - 2)), 4) / 4 + 1);

      std::string strVER = "";
      strVER += std::to_string(ndVER->major);
      strVER += ".";
      strVER += std::to_string(ndVER->minor);

      char *strVAL = (char*)value;
      memset(strVAL, 0x0, 64);
      memcpy(strVAL, strVER.c_str(), (std::min)(size_t(63), strVER.size()));

      break;
    }
    case HSA_CODE_OBJECT_INFO_ISA: {
      NDIsa *ndISA =
        (NDIsa*)(rawNOTE + RoundUp(size_t(*(rawNOTE - 2)), 4) / 4 + 1);

      std::string strISA = "";
      strISA += ndISA->ven;
      strISA += ":";
      strISA += ndISA->arc;
      strISA += ":";
      strISA += std::to_string(ndISA->major);
      strISA += ":";
      strISA += std::to_string(ndISA->minor);
      strISA += ":";
      strISA += std::to_string(ndISA->stepping);

      return hsa_isa_from_name(strISA.c_str(), (hsa_isa_t*)value);
    }
    case HSA_CODE_OBJECT_INFO_MACHINE_MODEL: {
      NDHsail *ndHSAIL =
        (NDHsail*)(rawNOTE + RoundUp(size_t(*(rawNOTE - 2)), 4) / 4 + 1);

      *((hsa_machine_model_t*)value) = hsa_machine_model_t(ndHSAIL->machine_model);
      break;
    }
    case HSA_CODE_OBJECT_INFO_PROFILE: {
      NDHsail *ndHSAIL =
        (NDHsail*)(rawNOTE + RoundUp(size_t(*(rawNOTE - 2)), 4) / 4 + 1);

      *((hsa_profile_t*)value) = hsa_profile_t(ndHSAIL->profile);
      break;
    }
    case HSA_CODE_OBJECT_INFO_DEFAULT_FLOAT_ROUNDING_MODE: {
      NDHsail *ndHSAIL =
        (NDHsail*)(rawNOTE + RoundUp(size_t(*(rawNOTE - 2)), 4) / 4 + 1);

      *((hsa_default_float_rounding_mode_t*)value) =
        hsa_default_float_rounding_mode_t(ndHSAIL->rounding_mode);
      break;
    }
    default: {
      assert(false);
    }
  }

  elf_end(amdgpuemi);
  return HSA_STATUS_SUCCESS;
}

std::unordered_map<void*, std::vector<Symbol*>*> AmdGpuImage::symbols_;

hsa_status_t AmdGpuImage::GetSymbol(void *amdgpumi,
                                    const char *module_name,
                                    const char *symbol_name,
                                    hsa_code_symbol_t *sym_handle) {
  assert(ElfMemoryImage::Is64(amdgpumi));
  assert(ElfMemoryImage::IsAmdGpu(amdgpumi));

  assert(symbol_name);
  assert(sym_handle);

  std::string mangled_name = std::string(symbol_name);
  if (module_name) {
    mangled_name.insert(0, "::");
    mangled_name.insert(0, std::string(module_name));
  }

  uint64_t amdgpumisz = ElfMemoryImage::Size(amdgpumi);
  if (!amdgpumisz) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  uint16_t symtabSNDX = ElfMemoryImage::FindSNDX(amdgpumi, SHT_SYMTAB);
  if (!symtabSNDX) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  Elf *amdgpuemi = NULL;
  amdgpuemi = elf_memory((char*)amdgpumi, size_t(amdgpumisz));
  assert(amdgpuemi);

  Elf_Scn *symtabSCN = NULL;
  symtabSCN = elf_getscn(amdgpuemi, size_t(symtabSNDX));
  assert(symtabSCN);

  Elf64_Shdr *symtabSHDR = NULL;
  symtabSHDR = elf64_getshdr(symtabSCN);
  assert(symtabSHDR);

  uint32_t strtabSNDX = symtabSHDR->sh_link;
  assert(strtabSNDX);

  Elf_Data *symtabDAT = NULL;
  symtabDAT = elf_getdata(symtabSCN, symtabDAT);
  assert(symtabDAT);

  Elf64_Sym *rawDAT = (Elf64_Sym*)symtabDAT->d_buf;
  assert(rawDAT);
  size_t rawSZ = symtabDAT->d_size;
  assert(rawSZ);
  assert(!(rawSZ % sizeof(Elf64_Sym)));

  auto symbol_entry = symbols_.find(amdgpumi);
  if (symbol_entry == symbols_.end()) {
    std::vector<Symbol*> *vec =
      new std::vector<Symbol*>(rawSZ / sizeof(Elf64_Sym), NULL);
    symbol_entry = symbols_.insert(std::make_pair(amdgpumi, vec)).first;
  }

  for (size_t i = 0; i < rawSZ / sizeof(Elf64_Sym); ++i) {
    char *elf_symbol_name = elf_strptr(amdgpuemi, strtabSNDX, rawDAT[i].st_name);
    assert(elf_symbol_name);

    if (mangled_name == std::string(elf_symbol_name)) {
      if (symbol_entry->second->at(i)) {
        *sym_handle = Symbol::CConvert(symbol_entry->second->at(i));
        elf_end(amdgpuemi);
        return HSA_STATUS_SUCCESS;
      }

      Elf_Scn *scn = NULL;
      scn = elf_getscn(amdgpuemi, size_t(rawDAT[i].st_shndx));
      assert(scn);
      Elf64_Shdr *shdr = NULL;
      shdr = elf64_getshdr(scn);
      assert(shdr);
      Elf_Data *data = NULL;
      data = elf_getdata(scn, data);
      assert(data);

      Symbol *sym = NULL;
      switch (ELF64_ST_TYPE(rawDAT[i].st_info)) {
        case STT_AMDGPU_HSA_KERNEL: {
          char *text = (char*)data->d_buf;
          assert(text);

          amd_kernel_code_t *akc =
            (amd_kernel_code_t*)(text + rawDAT[i].st_value);
          assert(akc);

          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = true;
          uint32_t kernarg_segment_size =
            uint32_t(akc->kernarg_segment_byte_size);
          uint32_t kernarg_segment_alignment =
            uint32_t(1 << akc->kernarg_segment_alignment);
          uint32_t group_segment_size =
            uint32_t(akc->workgroup_group_segment_byte_size);
          uint32_t private_segment_size =
            uint32_t(akc->workitem_private_segment_byte_size);
          bool is_dynamic_callstack =
            bool(akc->is_dynamic_callstack);

          sym = new KernelSymbol(false,
                                 mangled_name,
                                 linkage,
                                 is_definition,
                                 kernarg_segment_size,
                                 kernarg_segment_alignment,
                                 group_segment_size,
                                 private_segment_size,
                                 is_dynamic_callstack);
          break;
        }
        case STT_COMMON: {
          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = false;
          hsa_variable_allocation_t allocation =
            shdr->sh_flags & SHF_AMDGPU_HSA_AGENT ?
              HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
          hsa_variable_segment_t segment =
            shdr->sh_flags & SHF_AMDGPU_HSA_READONLY ?
              HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
          uint32_t size = uint32_t(rawDAT[i].st_size);
          uint32_t alignment = uint32_t(rawDAT[i].st_value);
          bool is_constant = shdr->sh_flags & SHF_WRITE ? true : false;

          sym = new VariableSymbol(false,
                                   mangled_name,
                                   linkage,
                                   is_definition,
                                   allocation,
                                   segment,
                                   size,
                                   alignment,
                                   is_constant);
          break;
        }
        case STT_OBJECT: {
          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = true;
          hsa_variable_allocation_t allocation =
            shdr->sh_flags & SHF_AMDGPU_HSA_AGENT ?
              HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
          hsa_variable_segment_t segment =
            shdr->sh_flags & SHF_AMDGPU_HSA_READONLY ?
              HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
          uint32_t size = uint32_t(rawDAT[i].st_size);
          uint32_t alignment = uint32_t(shdr->sh_addralign);
          bool is_constant = shdr->sh_flags & SHF_WRITE ? true : false;

          sym = new VariableSymbol(false,
                                   mangled_name,
                                   linkage,
                                   is_definition,
                                   allocation,
                                   segment,
                                   size,
                                   alignment,
                                   is_constant);
          break;
        }
        default: {
          elf_end(amdgpuemi);
          return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
        }
      }

      assert(!symbol_entry->second->at(i));
      symbol_entry->second->at(i) = sym;
      *sym_handle = Symbol::CConvert(symbol_entry->second->at(i));

      elf_end(amdgpuemi);
      return HSA_STATUS_SUCCESS;
    }
  }

  elf_end(amdgpuemi);
  return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
}

hsa_status_t AmdGpuImage::IterateSymbols(hsa_code_object_t code_object,
                                         hsa_status_t (*callback)(
                                           hsa_code_object_t code_object,
                                           hsa_code_symbol_t symbol,
                                           void* data),
                                         void* data) {
  assert(callback);

  void *amdgpumi = reinterpret_cast<void*>(code_object.handle);
  if (!amdgpumi) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (!core::loader::ElfMemoryImage::Is64(amdgpumi)) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (!core::loader::ElfMemoryImage::IsAmdGpu(amdgpumi)) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  uint64_t amdgpumisz = ElfMemoryImage::Size(amdgpumi);
  if (!amdgpumisz) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  uint16_t symtabSNDX = ElfMemoryImage::FindSNDX(amdgpumi, SHT_SYMTAB);
  if (!symtabSNDX) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  Elf *amdgpuemi = NULL;
  amdgpuemi = elf_memory((char*)amdgpumi, size_t(amdgpumisz));
  assert(amdgpuemi);

  Elf_Scn *symtabSCN = NULL;
  symtabSCN = elf_getscn(amdgpuemi, size_t(symtabSNDX));
  assert(symtabSCN);

  Elf64_Shdr *symtabSHDR = NULL;
  symtabSHDR = elf64_getshdr(symtabSCN);
  assert(symtabSHDR);

  uint32_t strtabSNDX = symtabSHDR->sh_link;
  assert(strtabSNDX);

  Elf_Data *symtabDAT = NULL;
  symtabDAT = elf_getdata(symtabSCN, symtabDAT);
  assert(symtabDAT);

  Elf64_Sym *rawDAT = (Elf64_Sym*)symtabDAT->d_buf;
  assert(rawDAT);
  size_t rawSZ = symtabDAT->d_size;
  assert(rawSZ);
  assert(!(rawSZ % sizeof(Elf64_Sym)));

  auto symbol_entry = symbols_.find(amdgpumi);
  if (symbol_entry == symbols_.end()) {
    std::vector<Symbol*> *vec =
      new std::vector<Symbol*>(rawSZ / sizeof(Elf64_Sym), NULL);
    symbol_entry = symbols_.insert(std::make_pair(amdgpumi, vec)).first;
  }

  for (size_t i = 0; i < rawSZ / sizeof(Elf64_Sym); ++i) {
    hsa_code_symbol_t sym_handle = {0};
    if (symbol_entry->second->at(i)) {
      sym_handle = Symbol::CConvert(symbol_entry->second->at(i));
    } else {
      char *elf_symbol_name =
        elf_strptr(amdgpuemi, strtabSNDX, rawDAT[i].st_name);
      assert(elf_symbol_name);

      std::string mangled_name(elf_symbol_name);

      Elf_Scn *scn = NULL;
      scn = elf_getscn(amdgpuemi, size_t(rawDAT[i].st_shndx));
      assert(scn);
      Elf64_Shdr *shdr = NULL;
      shdr = elf64_getshdr(scn);
      assert(shdr);
      Elf_Data *data = NULL;
      data = elf_getdata(scn, data);
      assert(data);

      Symbol *sym = NULL;
      switch (ELF64_ST_TYPE(rawDAT[i].st_info)) {
        case STT_AMDGPU_HSA_KERNEL: {
          char *text = (char*)data->d_buf;
          assert(text);

          amd_kernel_code_t *akc =
            (amd_kernel_code_t*)(text + rawDAT[i].st_value);
          assert(akc);

          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = true;
          uint32_t kernarg_segment_size =
            uint32_t(akc->kernarg_segment_byte_size);
          uint32_t kernarg_segment_alignment =
            uint32_t(1 << akc->kernarg_segment_alignment);
          uint32_t group_segment_size =
            uint32_t(akc->workgroup_group_segment_byte_size);
          uint32_t private_segment_size =
            uint32_t(akc->workitem_private_segment_byte_size);
          bool is_dynamic_callstack =
            bool(akc->is_dynamic_callstack);

          sym = new KernelSymbol(false,
                                 mangled_name,
                                 linkage,
                                 is_definition,
                                 kernarg_segment_size,
                                 kernarg_segment_alignment,
                                 group_segment_size,
                                 private_segment_size,
                                 is_dynamic_callstack);
          break;
        }
        case STT_COMMON: {
          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = false;
          hsa_variable_allocation_t allocation =
            shdr->sh_flags & SHF_AMDGPU_HSA_AGENT ?
              HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
          hsa_variable_segment_t segment =
            shdr->sh_flags & SHF_AMDGPU_HSA_READONLY ?
              HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
          uint32_t size = uint32_t(rawDAT[i].st_size);
          uint32_t alignment = uint32_t(rawDAT[i].st_value);
          bool is_constant = shdr->sh_flags & SHF_WRITE ? true : false;

          sym = new VariableSymbol(false,
                                   mangled_name,
                                   linkage,
                                   is_definition,
                                   allocation,
                                   segment,
                                   size,
                                   alignment,
                                   is_constant);
          break;
        }
        case STT_OBJECT: {
          hsa_symbol_linkage_t linkage =
            ELF64_ST_BIND(rawDAT[i].st_info) == STB_GLOBAL ?
              HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
          bool is_definition = true;
          hsa_variable_allocation_t allocation =
            shdr->sh_flags & SHF_AMDGPU_HSA_AGENT ?
              HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
          hsa_variable_segment_t segment =
            shdr->sh_flags & SHF_AMDGPU_HSA_READONLY ?
              HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
          uint32_t size = uint32_t(rawDAT[i].st_size);
          uint32_t alignment = uint32_t(shdr->sh_addralign);
          bool is_constant = shdr->sh_flags & SHF_WRITE ? true : false;

          sym = new VariableSymbol(false,
                                   mangled_name,
                                   linkage,
                                   is_definition,
                                   allocation,
                                   segment,
                                   size,
                                   alignment,
                                   is_constant);
          break;
        }
        default: {
          break;
        }
      }

      if (sym) {
        assert(!symbol_entry->second->at(i));
        symbol_entry->second->at(i) = sym;
        sym_handle = Symbol::CConvert(symbol_entry->second->at(i));
      }
    }

    if (sym_handle.handle) {
      hsa_status_t hsc = callback(code_object, sym_handle, data);
      if (HSA_STATUS_SUCCESS != hsc) {
        elf_end(amdgpuemi);
        return hsc;
      }
    }
  }

  elf_end(amdgpuemi);
  return HSA_STATUS_SUCCESS;
}

//===----------------------------------------------------------------------===//
// Symbol.                                                                    //
//===----------------------------------------------------------------------===//

hsa_code_symbol_t Symbol::CConvert(Symbol *sym) {
  hsa_code_symbol_t sym_handle;
  sym_handle.handle = reinterpret_cast<uint64_t>(sym);
  return sym_handle;
}

Symbol* Symbol::CConvert(hsa_code_symbol_t sym_handle) {
  return reinterpret_cast<Symbol*>(sym_handle.handle);
}

hsa_executable_symbol_t Symbol::EConvert(Symbol *sym) {
  hsa_executable_symbol_t sym_handle;
  sym_handle.handle = reinterpret_cast<uint64_t>(sym);
  return sym_handle;
}

Symbol* Symbol::EConvert(hsa_executable_symbol_t sym_handle) {
  return reinterpret_cast<Symbol*>(sym_handle.handle);
}

hsa_status_t Symbol::GetInfo(const symbol_attribute32_t &attribute, void *value) {
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_TYPE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_TYPE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_TYPE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_TYPE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_NAME_LENGTH) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_NAME) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_NAME)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_MODULE_NAME) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_LINKAGE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_LINKAGE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_IS_DEFINITION) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_IS_DEFINITION)),
    "attributes are not compatible"
  );

  assert(value);

  switch (attribute) {
    case HSA_CODE_SYMBOL_INFO_TYPE: {
      *((hsa_symbol_kind_t*)value) = kind;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_NAME_LENGTH: {
      std::string matter = "";

      if (linkage == HSA_SYMBOL_LINKAGE_PROGRAM) {
        assert(name.rfind(":") == std::string::npos);
        matter = name;
      } else {
        assert(name.rfind(":") != std::string::npos);
        matter = name.substr(name.rfind(":") + 1);
      }

      *((uint32_t*)value) = matter.size() + 1;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_NAME: {
      std::string matter = "";

      if (linkage == HSA_SYMBOL_LINKAGE_PROGRAM) {
        assert(name.rfind(":") == std::string::npos);
        matter = name;
      } else {
        assert(name.rfind(":") != std::string::npos);
        matter = name.substr(name.rfind(":") + 1);
      }

      memset(value, 0x0, matter.size() + 1);
      memcpy(value, matter.c_str(), matter.size());
      break;
    }
    case HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH: {
      std::string matter = "";

      if (linkage == HSA_SYMBOL_LINKAGE_PROGRAM) {
        assert(name.find(":") == std::string::npos);
        *((uint32_t*)value) = 0;
        return HSA_STATUS_SUCCESS;
      }

      assert(name.find(":") != std::string::npos);
      matter = name.substr(0, name.find(":"));

      *((uint32_t*)value) = matter.size() + 1;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_MODULE_NAME: {
      std::string matter = "";

      if (linkage == HSA_SYMBOL_LINKAGE_PROGRAM) {
        assert(name.find(":") == std::string::npos);
        return HSA_STATUS_SUCCESS;
      }

      assert(name.find(":") != std::string::npos);
      matter = name.substr(0, name.find(":"));

      memset(value, 0x0, matter.size() + 1);
      memcpy(value, matter.c_str(), matter.size());
      break;
    }
    case HSA_CODE_SYMBOL_INFO_LINKAGE: {
      *((hsa_symbol_linkage_t*)value) = linkage;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_IS_DEFINITION: {
      *((bool*)value) = is_definition;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT:
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS: {
      if (!is_loaded) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
      }
      *((uint64_t*)value) = address;
      break;
    }
    case HSA_EXECUTABLE_SYMBOL_INFO_AGENT: {
      if (!is_loaded) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
      }
      *((hsa_agent_t*)value) = agent;
      break;
    }
    default: {
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
  }

  return HSA_STATUS_SUCCESS;
}

//===----------------------------------------------------------------------===//
// KernelSymbol.                                                              //
//===----------------------------------------------------------------------===//

hsa_status_t KernelSymbol::GetInfo(const symbol_attribute32_t &attribute, void *value) {
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK)),
    "attributes are not compatible"
  );

  assert(value);

  switch (attribute) {
    case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE: {
      *((uint32_t*)value) = kernarg_segment_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT: {
      *((uint32_t*)value) = kernarg_segment_alignment;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE: {
      *((uint32_t*)value) = group_segment_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE: {
      *((uint32_t*)value) = private_segment_size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK: {
      *((bool*)value) = is_dynamic_callstack;
      break;
    }
    default: {
      return Symbol::GetInfo(attribute, value);
    }
  }

  return HSA_STATUS_SUCCESS;
}

//===----------------------------------------------------------------------===//
// VariableSymbol.                                                            //
//===----------------------------------------------------------------------===//

hsa_status_t VariableSymbol::GetInfo(const symbol_attribute32_t &attribute, void *value) {
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_VARIABLE_ALLOCATION) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALLOCATION)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_VARIABLE_SEGMENT) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SEGMENT)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_VARIABLE_SIZE) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE)),
    "attributes are not compatible"
  );
  static_assert(
    (symbol_attribute32_t(HSA_CODE_SYMBOL_INFO_VARIABLE_IS_CONST) ==
     symbol_attribute32_t(HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_IS_CONST)),
    "attributes are not compatible"
  );

  switch (attribute) {
    case HSA_CODE_SYMBOL_INFO_VARIABLE_ALLOCATION: {
      *((hsa_variable_allocation_t*)value) = allocation;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_SEGMENT: {
      *((hsa_variable_segment_t*)value) = segment;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT: {
      *((uint32_t*)value) = alignment;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_SIZE: {
      *((uint32_t*)value) = size;
      break;
    }
    case HSA_CODE_SYMBOL_INFO_VARIABLE_IS_CONST: {
      *((bool*)value) = is_constant;
      break;
    }
    default: {
      return Symbol::GetInfo(attribute, value);
    }
  }

  return HSA_STATUS_SUCCESS;
}

//===----------------------------------------------------------------------===//
// Executable.                                                                //
//===----------------------------------------------------------------------===//

hsa_executable_t Executable::Convert(Executable *exec) {
  hsa_executable_t exec_handle;
  exec_handle.handle = reinterpret_cast<uint64_t>(exec);
  return exec_handle;
}

Executable* Executable::Convert(hsa_executable_t exec_handle) {
  return reinterpret_cast<Executable*>(exec_handle.handle);
}

Executable::Executable(const hsa_profile_t &_profile,
                       const hsa_executable_state_t &_state)
  : profile_(_profile)
  , state_(_state) {
  alsegs_.fill(NULL);
}

Executable::~Executable() {
  if (alsegs_[ALSEG_NDX_CA]) {
    for (auto &maddr : *(alsegs_[ALSEG_NDX_CA])) {
      DeallocateCodeMemory(reinterpret_cast<void*>(maddr->addr), maddr->size);
      delete maddr;
    }
    delete alsegs_[ALSEG_NDX_CA];
    alsegs_[ALSEG_NDX_CA] = NULL;
  }

  if (alsegs_[ALSEG_NDX_GP]) {
    assert(1 == alsegs_[ALSEG_NDX_GP]->size());
    hsa_memory_free(reinterpret_cast<void*>(alsegs_[ALSEG_NDX_GP]->back()->addr));
    delete alsegs_[ALSEG_NDX_GP]->back();
    delete alsegs_[ALSEG_NDX_GP];
    alsegs_[ALSEG_NDX_GP] = NULL;
  }

  for (auto &mr : alsegs_) {
    if (mr) {
      for (auto &maddr : *mr) {
        hsa_memory_free(reinterpret_cast<void*>(maddr->addr));
        delete maddr;
      }
      delete mr;
    }
  }
  alsegs_.fill(NULL);

  for (auto &symbol_entry : program_symbols_) {
    delete symbol_entry.second;
  }
  for (auto &symbol_entry : agent_symbols_) {
    delete symbol_entry.second;
  }
  for (auto &sampler : samplers_) {
    hsa_ext_sampler_destroy(sampler.owner, sampler.handle);
  }
  for (auto &image : images_) {
    hsa_ext_image_destroy(image.owner, image.handle);
  }
  for (auto &debug_info : debug_info_) {
    delete debug_info;
  }
}

hsa_status_t Executable::DefineProgramVariable(const char *var_name,
                                               void *address) {
  assert(var_name);
  assert(address);

  if (HSA_EXECUTABLE_STATE_FROZEN == state_) {
    return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
  }

  auto symbol_entry = program_symbols_.find(std::string(var_name));
  if (symbol_entry != program_symbols_.end()) {
    return HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
  }

  program_symbols_.insert(
    std::make_pair(std::string(var_name),
                   new VariableSymbol(true,
                                      std::string(var_name),
                                      HSA_SYMBOL_LINKAGE_PROGRAM,
                                      true,
                                      HSA_VARIABLE_ALLOCATION_PROGRAM,
                                      HSA_VARIABLE_SEGMENT_GLOBAL,
                                      0,     // TODO: size.
                                      0,     // TODO: align.
                                      false, // TODO: const.
                                      true,
                                      reinterpret_cast<uint64_t>(address),
                                      reinterpret_cast<uint64_t>(address))));
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::DefineAgentVariable(const char *var_name,
                                             const hsa_agent_t &agent,
                                             const hsa_variable_segment_t &segment,
                                             void *address) {
  assert(var_name);
  assert(address);

  if (HSA_EXECUTABLE_STATE_FROZEN == state_) {
    return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
  }

  auto symbol_entry = agent_symbols_.find(std::make_pair(std::string(var_name), agent));
  if (symbol_entry != agent_symbols_.end()) {
    return HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
  }

  agent_symbols_.insert(
    std::make_pair(std::make_pair(std::string(var_name), agent),
                   new VariableSymbol(true,
                                      std::string(var_name),
                                      HSA_SYMBOL_LINKAGE_PROGRAM,
                                      true,
                                      HSA_VARIABLE_ALLOCATION_AGENT,
                                      segment,
                                      0,     // TODO: size.
                                      0,     // TODO: align.
                                      false, // TODO: const.
                                      true,
                                      reinterpret_cast<uint64_t>(address),
                                      reinterpret_cast<uint64_t>(address))));
  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::GetSymbol(const char *module_name,
                                   const char *symbol_name,
                                   const hsa_agent_t &agent,
                                   const int32_t &call_convention,
                                   hsa_executable_symbol_t *symbol) {
  assert(symbol_name);
  assert(symbol);

  std::string mangled_name = std::string(symbol_name);
  if (module_name) {
    mangled_name.insert(0, "::");
    mangled_name.insert(0, std::string(module_name));
  }

  // TODO(spec): this is not spec compliant.
  if (!agent.handle) {
    auto program_symbol = program_symbols_.find(mangled_name);
    if (program_symbol != program_symbols_.end()) {
      *symbol = Symbol::EConvert(program_symbol->second);
      return HSA_STATUS_SUCCESS;
    }
  } else {
    auto agent_symbol = agent_symbols_.find(std::make_pair(mangled_name, agent));
    if (agent_symbol != agent_symbols_.end()) {
      *symbol = Symbol::EConvert(agent_symbol->second);
      return HSA_STATUS_SUCCESS;
    }
  }

  return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
}

hsa_status_t Executable::IterateSymbols(hsa_status_t (*callback)(
                                          hsa_executable_t executable,
                                          hsa_executable_symbol_t symbol,
                                          void* data),
                                        void* data) {
  assert(callback);

  for (auto &symbol_entry : program_symbols_) {
    hsa_status_t hsc =
      callback(Convert(this), Symbol::EConvert(symbol_entry.second), data);
    if (HSA_STATUS_SUCCESS != hsc) {
      return hsc;
    }
  }
  for (auto &symbol_entry : agent_symbols_) {
    hsa_status_t hsc =
      callback(Convert(this), Symbol::EConvert(symbol_entry.second), data);
    if (HSA_STATUS_SUCCESS != hsc) {
      return hsc;
    }
  }

  return HSA_STATUS_SUCCESS;
}

#define HSAERRCHECK(hsc)                                                       \
  if (hsc != HSA_STATUS_SUCCESS) {                                             \
    assert(false);                                                             \
    return hsc;                                                                \
  }                                                                            \

hsa_status_t Executable::LoadCodeObject(const hsa_agent_t &agent,
                                        const hsa_code_object_t &code_object,
                                        const char *options) {
  if (HSA_EXECUTABLE_STATE_FROZEN == state_) {
    return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
  }

  void *elfmemrd = reinterpret_cast<void*>(code_object.handle);
  if (!elfmemrd) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (!core::loader::ElfMemoryImage::Is64(elfmemrd)) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }
  if (!core::loader::ElfMemoryImage::IsAmdGpu(elfmemrd)) {
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  hsa_isa_t objectsIsa = {0};
  hsa_status_t hsc = AmdGpuImage::GetInfo(elfmemrd, HSA_CODE_OBJECT_INFO_ISA, &objectsIsa);
  HSAERRCHECK(hsc);

  hsa_isa_t agentsIsa = {0};
  hsc = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &agentsIsa);
  HSAERRCHECK(hsc);

  bool isCompatible = false;
  hsc = hsa_isa_compatible(objectsIsa, agentsIsa, &isCompatible);
  HSAERRCHECK(hsc);

  if (!isCompatible) {
    assert(false);
    return HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS;
  }

  uint64_t elfmemsz = ElfMemoryImage::Size(elfmemrd);
  if (!elfmemsz) {
    assert(false);
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
  }

  //===--------------------------------------------------------------------===//

  Elf *elf = elf_memory((char*)elfmemrd, size_t(elfmemsz));
  assert(elf);

  bool new_prog_region = false;
  size_t stndx = SIZE_MAX;
  std::list<size_t> relndx;

  //<<<
  Elf64_Ehdr *ehdr = elf64_getehdr(elf);
  assert(ehdr);

  uint16_t phnum = ehdr->e_phnum;
  assert(phnum);

  Elf64_Phdr *phdrs = elf64_getphdr(elf);
  assert(phdrs);

  for (uint16_t i = 0; i < phnum; ++i) {
    size_t alseg_ndx = SIZE_MAX;
    switch (phdrs[i].p_type) {
      case PT_AMDGPU_HSA_LOAD_GLOBAL_PROGRAM: {
        alseg_ndx = ALSEG_NDX_GP;
        break;
      }
      case PT_AMDGPU_HSA_LOAD_GLOBAL_AGENT: {
        alseg_ndx = ALSEG_NDX_GA;
        break;
      }
      case PT_AMDGPU_HSA_LOAD_READONLY_AGENT: {
        alseg_ndx = ALSEG_NDX_RA;
        break;
      }
      case PT_AMDGPU_HSA_LOAD_CODE_AGENT: {
        alseg_ndx = ALSEG_NDX_CA;
        break;
      }
      default: {
        break;
      }
    }
    assert(SIZE_MAX != alseg_ndx);

    hsc = AlsegCreate(alseg_ndx, phdrs[i].p_vaddr, agent, phdrs[i].p_memsz,
                      phdrs[i].p_align, new_prog_region);
    HSAERRCHECK(hsc);

    if (alseg_ndx == ALSEG_NDX_GP && !new_prog_region) {
      continue;
    }

    // TODO(kzhuravl): remove following block once hsa runtime has an api
    // to allocate zero-initialized memory.
    void *data = calloc(phdrs[i].p_memsz, 1);
    assert(data);
    memcpy(data, (char*)elfmemrd + phdrs[i].p_offset, phdrs[i].p_filesz);

    assert(alsegs_[alseg_ndx]);
    assert(alsegs_[alseg_ndx]->size());
    assert(alsegs_[alseg_ndx]->back());

    MemoryAddress *symad = alsegs_[alseg_ndx]->back();
    assert(symad);
    memcpy(reinterpret_cast<void*>(symad->addr), data, phdrs[i].p_memsz);
  }
  //<<<

  //<<<
  Elf_Scn *scn = NULL;
  Elf64_Shdr *shdr = NULL;
  Elf_Data *data = NULL;
  while (NULL != (scn = elf_nextscn(elf, scn))) {
    shdr = NULL;
    data = NULL;

    shdr = elf64_getshdr(scn);
    assert(shdr);

    switch (shdr->sh_type) {
      case SHT_RELA: {
        relndx.push_back(elf_ndxscn(scn));
        break;
      }
      case SHT_SYMTAB: {
        stndx = elf_ndxscn(scn);
        break;
      }
      default: {
        break;
      }
    }
  }
  //<<<

  //<<<
  assert(SIZE_MAX != stndx);

  scn = NULL;
  scn = elf_getscn(elf, stndx);
  assert(scn);

  shdr = NULL;
  shdr = elf64_getshdr(scn);
  assert(shdr);

  data = NULL;
  data = elf_getdata(scn, data);
  assert(data);

  Elf64_Sym *symdt = (Elf64_Sym*)data->d_buf;
  assert(symdt);

  uint64_t symsz = data->d_size;
  assert(symsz);
  assert(!(symsz % sizeof(Elf64_Sym)));

  for (uint64_t i = 0; i < symsz / sizeof(Elf64_Sym); ++i) {
    char *symnm =
      elf_strptr(elf, size_t(shdr->sh_link), size_t(symdt[i].st_name));
    assert(symnm);

    if (STT_OBJECT == ELF64_ST_TYPE(symdt[i].st_info) ||
        STT_AMDGPU_HSA_KERNEL == ELF64_ST_TYPE(symdt[i].st_info)) {
      Elf_Scn *def_scn = NULL;
      def_scn = elf_getscn(elf, size_t(symdt[i].st_shndx));
      assert(def_scn);

      Elf64_Shdr *def_shdr = NULL;
      def_shdr = elf64_getshdr(def_scn);
      assert(def_shdr);

      if (def_shdr->sh_flags & SHF_AMDGPU_HSA_AGENT) {
        auto agent_symbol = agent_symbols_.find(std::make_pair(std::string(symnm), agent));
        if (agent_symbol != agent_symbols_.end()) {
          // TODO(spec): this is not spec compliant.
          return HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
        }
      } else {
        auto program_symbol = program_symbols_.find(std::string(symnm));
        if (program_symbol != program_symbols_.end()) {
          // TODO(spec): this is not spec compliant.
          return HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
        }
      }

      size_t alseg_ndx = Alsec2AlsegNdx(def_shdr->sh_type, def_shdr->sh_flags);
      assert(SIZE_MAX != alseg_ndx);

      if (alseg_ndx == ALSEG_NDX_GP && !new_prog_region) {
        continue;
      }

      assert(alsegs_[alseg_ndx]);
      assert(alsegs_[alseg_ndx]->size());
      assert(alsegs_[alseg_ndx]->back());

      MemoryAddress *symad = alsegs_[alseg_ndx]->back();
      assert(symad);
      uint64_t symad_base = symad->addr + (def_shdr->sh_addr - symad->p_vaddr);

      Symbol *symbob = NULL;
      if (STT_OBJECT == ELF64_ST_TYPE(symdt[i].st_info)) {
        hsa_symbol_linkage_t linkage =
          ELF64_ST_BIND(symdt[i].st_info) == STB_GLOBAL ?
            HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
        bool is_definition = true;
        hsa_variable_allocation_t allocation =
          def_shdr->sh_flags & SHF_AMDGPU_HSA_AGENT ?
            HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
        hsa_variable_segment_t segment =
          def_shdr->sh_flags & SHF_AMDGPU_HSA_READONLY ?
            HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
        uint32_t size = uint32_t(symdt[i].st_size);
        uint32_t alignment = uint32_t(def_shdr->sh_addralign);
        bool is_constant = def_shdr->sh_flags & SHF_WRITE ? true : false;
        bool is_external = false;
        uint64_t address = symad_base + symdt[i].st_value;

        symbob = new VariableSymbol(true,
                                    std::string(symnm),
                                    linkage,
                                    is_definition,
                                    allocation,
                                    segment,
                                    size,
                                    alignment,
                                    is_constant,
                                    false,
                                    symad_base,
                                    address);
      } else if (STT_AMDGPU_HSA_KERNEL == ELF64_ST_TYPE(symdt[i].st_info)) {
        amd_kernel_code_t *akc =
          (amd_kernel_code_t*)(symad_base + symdt[i].st_value);
        assert(akc);

        DebugInfo *debug_info = new DebugInfo(elfmemrd, size_t(elfmemsz));
        akc->runtime_loader_kernel_symbol =
          reinterpret_cast<uint64_t>(debug_info);
        debug_info_.push_back(debug_info);

        hsa_symbol_linkage_t linkage =
          ELF64_ST_BIND(symdt[i].st_info) == STB_GLOBAL ?
            HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
        bool is_definition = true;
        uint32_t kernarg_segment_size =
          uint32_t(akc->kernarg_segment_byte_size);
        uint32_t kernarg_segment_alignment =
          uint32_t(1 << akc->kernarg_segment_alignment);
        uint32_t group_segment_size =
          uint32_t(akc->workgroup_group_segment_byte_size);
        uint32_t private_segment_size =
          uint32_t(akc->workitem_private_segment_byte_size);
        bool is_dynamic_callstack =
          bool(akc->is_dynamic_callstack);

        symbob = new KernelSymbol(true,
                                  std::string(symnm),
                                  linkage,
                                  is_definition,
                                  kernarg_segment_size,
                                  kernarg_segment_alignment,
                                  group_segment_size,
                                  private_segment_size,
                                  is_dynamic_callstack,
                                  symad_base,
                                  reinterpret_cast<uint64_t>(akc));
      } else {
        assert(false);
      }
      assert(symbob);

      if (def_shdr->sh_flags & SHF_AMDGPU_HSA_AGENT) {
        agent_symbols_.insert(
          std::make_pair(std::make_pair(std::string(symnm), agent), symbob));
      } else {
        program_symbols_.insert(std::make_pair(std::string(symnm), symbob));
      }
    } else if (STT_COMMON == ELF64_ST_TYPE(symdt[i].st_info)) {
      auto program_symbol = program_symbols_.find(std::string(symnm));
      if (program_symbol == program_symbols_.end()) {
        auto agent_symbol = agent_symbols_.find(std::make_pair(std::string(symnm), agent));
        if (agent_symbol == agent_symbols_.end()) {
          // TODO(spec): this is not spec compliant.
          return HSA_STATUS_ERROR_VARIABLE_UNDEFINED;
        }
      }
    }
  }
  //<<<

  //<<<
  for (auto &reloc : relndx) {
    Elf_Scn *relscn = NULL;
    relscn = elf_getscn(elf, reloc);
    assert(relscn);

    Elf64_Shdr *relshdr = NULL;
    relshdr = elf64_getshdr(relscn);
    assert(relshdr);

    Elf_Data *reldata = NULL;
    reldata = elf_getdata(relscn, reldata);
    assert(reldata);

    Elf64_Rela *rd = (Elf64_Rela*)reldata->d_buf;
    assert(rd);
    uint64_t rs = reldata->d_size;
    assert(rs);
    assert(!(rs % sizeof(Elf64_Rela)));

    Elf_Scn *stscn = NULL;
    stscn = elf_getscn(elf, size_t(relshdr->sh_link));
    assert(stscn);

    Elf64_Shdr *stshdr = NULL;
    stshdr = elf64_getshdr(stscn);
    assert(stshdr);

    Elf_Data *stdata = NULL;
    stdata = elf_getdata(stscn, stdata);
    assert(stdata);

    Elf64_Sym *sd = (Elf64_Sym*)stdata->d_buf;
    assert(sd);
    uint64_t ss = stdata->d_size;
    assert(ss);
    assert(!(ss % sizeof(Elf64_Sym)));

    Elf_Scn *dtscn = NULL;
    dtscn = elf_getscn(elf, size_t(relshdr->sh_info));
    assert(dtscn);

    Elf64_Shdr *dtshdr = NULL;
    dtshdr = elf64_getshdr(dtscn);
    assert(dtshdr);

    Elf_Data *dtdata = NULL;
    dtdata = elf_getdata(dtscn, dtdata);
    assert(dtdata);

    size_t alseg_ndx = Alsec2AlsegNdx(dtshdr->sh_type, dtshdr->sh_flags);
    assert(SIZE_MAX != alseg_ndx);
    if (alseg_ndx == ALSEG_NDX_GP && !new_prog_region) {
      continue;
    }
    assert(alsegs_[alseg_ndx]);
    assert(alsegs_[alseg_ndx]->size());
    assert(alsegs_[alseg_ndx]->back());

    MemoryAddress *mad = alsegs_[alseg_ndx]->back();
    assert(mad);
    uint64_t mad_base = mad->addr + (dtshdr->sh_addr - mad->p_vaddr);

    for (uint64_t i = 0; i < rs / sizeof(Elf64_Rela); ++i) {
      switch (ELF64_R_TYPE(rd[i].r_info)) {
        case R_AMDGPU_INIT_SAMPLER: {
          Elf64_Sym sis = sd[ELF64_R_SYM(rd[i].r_info)];
          assert(STT_AMDGPU_HSA_METADATA == ELF64_ST_TYPE(sis.st_info));

          Elf_Scn *sis_scn = NULL;
          sis_scn = elf_getscn(elf, size_t(sis.st_shndx));
          assert(sis_scn);

          Elf64_Shdr *sis_shdr = NULL;
          sis_shdr = elf64_getshdr(sis_scn);
          assert(sis_shdr);
          assert(SHT_PROGBITS == sis_shdr->sh_type);
          assert(SHF_MERGE == sis_shdr->sh_flags);

          Elf_Data *sis_data = NULL;
          sis_data = elf_getdata(sis_scn, sis_data);
          assert(sis_data);

          amdgpu_hsa_sampler_descriptor_t *sdd =
            (amdgpu_hsa_sampler_descriptor_t*)((char*)sis_data->d_buf + sis.st_value);
          assert(sdd);
          assert(sizeof(amdgpu_hsa_sampler_descriptor_t) == sdd->size);
          assert(AMDGPU_HSA_METADATA_KIND_INIT_SAMP == sdd->kind);

          hsa_ext_sampler_descriptor_t hsa_sampler_descriptor;
          hsa_sampler_descriptor.coordinate_mode =
            hsa_ext_sampler_coordinate_mode_t(sdd->coord);
          hsa_sampler_descriptor.filter_mode =
            hsa_ext_sampler_filter_mode_t(sdd->filter);
          hsa_sampler_descriptor.address_mode =
            hsa_ext_sampler_addressing_mode_t(sdd->addressing);

          hsa_ext_sampler_t hsa_sampler = {0};
          hsc = hsa_ext_sampler_create(agent, &hsa_sampler_descriptor, &hsa_sampler);
          HSAERRCHECK(hsc);
          assert(hsa_sampler.handle);

          memcpy(reinterpret_cast<void*>(mad_base + rd[i].r_offset),
                 &hsa_sampler,
                 sizeof(hsa_ext_sampler_t));
          samplers_.push_back(Sampler(agent, hsa_sampler));

          break;
        }
        case R_AMDGPU_INIT_IMAGE: {
          Elf64_Sym iis = sd[ELF64_R_SYM(rd[i].r_info)];
          assert(STT_AMDGPU_HSA_METADATA == ELF64_ST_TYPE(iis.st_info));

          Elf_Scn *iis_scn = NULL;
          iis_scn = elf_getscn(elf, size_t(iis.st_shndx));
          assert(iis_scn);

          Elf64_Shdr *iis_shdr = NULL;
          iis_shdr = elf64_getshdr(iis_scn);
          assert(iis_shdr);
          assert(SHT_PROGBITS == iis_shdr->sh_type);
          assert(SHF_MERGE == iis_shdr->sh_flags);

          Elf_Data *iis_data = NULL;
          iis_data = elf_getdata(iis_scn, iis_data);
          assert(iis_data);

          amdgpu_hsa_image_descriptor_t *idd =
            (amdgpu_hsa_image_descriptor_t*)((char*)iis_data->d_buf + iis.st_value);
          assert(idd);
          assert(sizeof(amdgpu_hsa_image_descriptor_t) == idd->size);
          assert(AMDGPU_HSA_METADATA_KIND_INIT_ROIMG == idd->kind ||
                 AMDGPU_HSA_METADATA_KIND_INIT_WOIMG == idd->kind ||
                 AMDGPU_HSA_METADATA_KIND_INIT_RWIMG == idd->kind);

          hsa_ext_image_format_t hsa_image_format;
          hsa_image_format.channel_order =
            hsa_ext_image_channel_order_t(idd->channel_order);
          hsa_image_format.channel_type =
            hsa_ext_image_channel_type_t(idd->channel_type);

          hsa_ext_image_descriptor_t hsa_image_descriptor;
          hsa_image_descriptor.geometry =
            hsa_ext_image_geometry_t(idd->geometry);
          hsa_image_descriptor.width = size_t(idd->width);
          hsa_image_descriptor.height = size_t(idd->height);
          hsa_image_descriptor.depth = size_t(idd->depth);
          hsa_image_descriptor.array_size = size_t(idd->array);
          hsa_image_descriptor.format = hsa_image_format;

          hsa_access_permission_t hsa_image_permission = HSA_ACCESS_PERMISSION_RO;
          switch (idd->kind) {
            case AMDGPU_HSA_METADATA_KIND_INIT_ROIMG: {
              hsa_image_permission = HSA_ACCESS_PERMISSION_RO;
              break;
            }
            case AMDGPU_HSA_METADATA_KIND_INIT_WOIMG: {
              hsa_image_permission = HSA_ACCESS_PERMISSION_WO;
              break;
            }
            case AMDGPU_HSA_METADATA_KIND_INIT_RWIMG: {
              hsa_image_permission = HSA_ACCESS_PERMISSION_RW;
              break;
            }
            default: {
              assert(false);
            }
          }

          hsa_ext_image_t hsa_image = {0};
          hsc = hsa_ext_image_create(agent,
                                     &hsa_image_descriptor,
                                     NULL, // TODO: image_data?
                                     hsa_image_permission,
                                     &hsa_image);
          HSAERRCHECK(hsc);
          assert(hsa_image.handle);

          memcpy(reinterpret_cast<void*>(mad_base + rd[i].r_offset),
                 &hsa_image,
                 sizeof(hsa_ext_image_t));
          images_.push_back(Image(agent, hsa_image));

          break;
        }
        default: {
          break;
        }
      }
    }
  }
  //<<<

  //<<<
  for (auto &reloc : relndx) {
    Elf_Scn *relscn = NULL;
    relscn = elf_getscn(elf, reloc);
    assert(relscn);

    Elf64_Shdr *relshdr = NULL;
    relshdr = elf64_getshdr(relscn);
    assert(relshdr);

    Elf_Data *reldata = NULL;
    reldata = elf_getdata(relscn, reldata);
    assert(reldata);

    Elf64_Rela *rd = (Elf64_Rela*)reldata->d_buf;
    assert(rd);
    uint64_t rs = reldata->d_size;
    assert(rs);
    assert(!(rs % sizeof(Elf64_Rela)));

    Elf_Scn *stscn = NULL;
    stscn = elf_getscn(elf, size_t(relshdr->sh_link));
    assert(stscn);

    Elf64_Shdr *stshdr = NULL;
    stshdr = elf64_getshdr(stscn);
    assert(stshdr);

    Elf_Data *stdata = NULL;
    stdata = elf_getdata(stscn, stdata);
    assert(stdata);

    Elf64_Sym *sd = (Elf64_Sym*)stdata->d_buf;
    assert(sd);
    uint64_t ss = stdata->d_size;
    assert(ss);
    assert(!(ss % sizeof(Elf64_Sym)));

    Elf_Scn *dtscn = NULL;
    dtscn = elf_getscn(elf, size_t(relshdr->sh_info));
    assert(dtscn);

    Elf64_Shdr *dtshdr = NULL;
    dtshdr = elf64_getshdr(dtscn);
    assert(dtshdr);

    Elf_Data *dtdata = NULL;
    dtdata = elf_getdata(dtscn, dtdata);
    assert(dtdata);

    size_t alseg_ndx = Alsec2AlsegNdx(dtshdr->sh_type, dtshdr->sh_flags);
    assert(SIZE_MAX != alseg_ndx);
      if (alseg_ndx == ALSEG_NDX_GP && !new_prog_region) {
        continue;
      }
    assert(alsegs_[alseg_ndx]);
    assert(alsegs_[alseg_ndx]->size());
    assert(alsegs_[alseg_ndx]->back());

    MemoryAddress *mad = alsegs_[alseg_ndx]->back();
    assert(mad);
    uint64_t mad_base = mad->addr + (dtshdr->sh_addr - mad->p_vaddr);

    for (uint64_t i = 0; i < rs / sizeof(Elf64_Rela); ++i) {
      switch (ELF64_R_TYPE(rd[i].r_info)) {
        case R_AMDGPU_32_LOW:
        case R_AMDGPU_32_HIGH: {
          Elf64_Sym vs = sd[ELF64_R_SYM(rd[i].r_info)];
          assert(STT_SECTION == ELF64_ST_TYPE(vs.st_info) ||
                 STT_COMMON == ELF64_ST_TYPE(vs.st_info));

          char *symnm =
            elf_strptr(elf, size_t(stshdr->sh_link), size_t(vs.st_name));
          assert(symnm);

          uint64_t base = 0;
          if (STT_SECTION == ELF64_ST_TYPE(vs.st_info)) {
            Elf_Scn *rscn = NULL;
            rscn = elf_getscn(elf, size_t(vs.st_shndx));
            assert(rscn);

            Elf64_Shdr *rshdr = NULL;
            rshdr = elf64_getshdr(rscn);
            assert(rshdr);

            size_t rndx = Alsec2AlsegNdx(rshdr->sh_type, rshdr->sh_flags);
            assert(SIZE_MAX != rndx);

            assert(alsegs_[rndx]);
            assert(alsegs_[rndx]->size());
            assert(alsegs_[rndx]->back());

            base = alsegs_[rndx]->back()->addr +
                     (rshdr->sh_addr - alsegs_[rndx]->back()->p_vaddr);
          } else {
            auto program_symbol =
              program_symbols_.find(std::string(symnm));
            if (program_symbol == program_symbols_.end()) {
              auto agent_symbol =
                agent_symbols_.find(std::make_pair(std::string(symnm), agent));
              if (agent_symbol == agent_symbols_.end()) {
                // TODO(spec): this is not spec compliant.
                return HSA_STATUS_ERROR_VARIABLE_UNDEFINED;
              } else {
                base = agent_symbol->second->base;
              }
            } else {
              base = program_symbol->second->base;
            }
          }
          assert(base);

          uint64_t vaddr = base + uint64_t(rd[i].r_addend);
          uint32_t raddr = 0;
          if (R_AMDGPU_32_HIGH == ELF64_R_TYPE(rd[i].r_info)) {
            raddr = uint32_t((vaddr >> 32) & 0xFFFFFFFF);
          } else if (R_AMDGPU_32_LOW == ELF64_R_TYPE(rd[i].r_info)) {
            raddr = uint32_t(vaddr & 0xFFFFFFFF);
          } else {
            assert(false);
          }

          memcpy(reinterpret_cast<void*>(mad_base + rd[i].r_offset), &raddr, sizeof(raddr));

          break;
        }
        default: {
          break;
        }
      }
    }
  }
  //<<<

  elf_end(elf);

  //===--------------------------------------------------------------------===//

  return HSA_STATUS_SUCCESS;
}

hsa_status_t Executable::Validate(uint32_t *result) {
  assert(result);
  *result = 0;
  return HSA_STATUS_SUCCESS;
}

size_t Executable::Alsec2AlsegNdx(const uint32_t &type, const uint64_t &flags) {
  if (SHT_NOBITS != type && SHT_PROGBITS != type) {
    return SIZE_MAX;
  }
  if (!(flags & SHF_ALLOC)) {
    return SIZE_MAX;
  }

  if (flags & SHF_EXECINSTR) {
    assert(flags & SHF_AMDGPU_HSA_AGENT);
    assert(flags & SHF_AMDGPU_HSA_CODE);
    return ALSEG_NDX_CA;
  }

  if ((flags & SHF_AMDGPU_HSA_GLOBAL) && (flags & SHF_AMDGPU_HSA_AGENT)) {
    return ALSEG_NDX_GA;
  } else if (flags & SHF_AMDGPU_HSA_GLOBAL) {
    return ALSEG_NDX_GP;
  }

  if (flags & SHF_AMDGPU_HSA_READONLY) {
    assert(flags & SHF_AMDGPU_HSA_AGENT);
    return ALSEG_NDX_RA;
  }

  return SIZE_MAX;
}

hsa_status_t Executable::AlsegCreate(const size_t &alseg_ndx,
                                     const uint64_t &p_vaddr,
                                     const hsa_agent_t &agent,
                                     const uint64_t &size,
                                     const uint64_t &align,
                                     bool &new_prog_region) {
  assert(ALSEG_CNT_MAX > alseg_ndx);
  assert(size);

  if (ALSEG_NDX_CA == alseg_ndx) {
    if (!alsegs_[alseg_ndx]) {
      alsegs_[alseg_ndx] = new std::list<MemoryAddress*>();
    }

    void *code_region_ptr = AllocateCodeMemory(size);
    if (!code_region_ptr) {
      return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
    }

    uint64_t code_region_adr = reinterpret_cast<uint64_t>(code_region_ptr);
    assert(!(code_region_adr % align));

    alsegs_[alseg_ndx]->push_back(
      new MemoryAddress(code_region_adr, p_vaddr, size, agent));
    return HSA_STATUS_SUCCESS;
  }

  if (ALSEG_NDX_GP == alseg_ndx) {
    if (!alsegs_[alseg_ndx]) {
      alsegs_[alseg_ndx] = new std::list<MemoryAddress*>();

      hsa_region_t program_region = {0};
      hsa_status_t hsc = hsa_agent_iterate_regions(agent, FindGlobalRegion, &program_region);
      HSAERRCHECK(hsc);
      assert(program_region.handle);

      void *program_region_ptr = NULL;
      hsc = hsa_memory_allocate(program_region, size, &program_region_ptr);
      HSAERRCHECK(hsc);
      assert(program_region_ptr);

      uint64_t program_region_adr = reinterpret_cast<uint64_t>(program_region_ptr);
      assert(!(program_region_adr % align));

      alsegs_[alseg_ndx]->push_back(
        new MemoryAddress(program_region_adr, p_vaddr, size, agent));
      new_prog_region = true;
    }

    assert(alsegs_[alseg_ndx]);
    assert(1 == alsegs_[alseg_ndx]->size());
    return HSA_STATUS_SUCCESS;
  }

  if (!alsegs_[alseg_ndx]) {
    alsegs_[alseg_ndx] = new std::list<MemoryAddress*>();
  }

  hsa_region_t agent_region = {0};
  hsa_status_t hsc = hsa_agent_iterate_regions(agent, FindGlobalRegion, &agent_region);
  HSAERRCHECK(hsc);
  assert(agent_region.handle);

  void *agent_region_ptr = NULL;
  hsc = hsa_memory_allocate(agent_region, size, &agent_region_ptr);
  HSAERRCHECK(hsc);
  assert(agent_region_ptr);

  uint64_t agent_region_adr = reinterpret_cast<uint64_t>(agent_region_ptr);
  assert(!(agent_region_adr % align));

  alsegs_[alseg_ndx]->push_back(
    new MemoryAddress(agent_region_adr, p_vaddr, size, agent));
  return HSA_STATUS_SUCCESS;
}

} // namespace loader
} // namespace core
