#ifndef HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_
#define HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_

#include <array>
#include <cassert>
#include <cstdint>
#include <libelf.h>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "hsa.h"
#include "hsa_ext_image.h"

#define ALSEG_CNT_MAX 4
#define ALSEG_NDX_GA  0
#define ALSEG_NDX_RA  1
#define ALSEG_NDX_GP  2
#define ALSEG_NDX_CA  3

namespace core {
namespace loader {

class MemoryAddress;
class ElfMemoryImage;
class AmdGpuImage;
class Symbol;
class KernelSymbol;
class VariableSymbol;
class Executable;

//===----------------------------------------------------------------------===//
// ElfMemoryImage.                                                            //
//===----------------------------------------------------------------------===//

class ElfMemoryImage final {
public:
  static uint16_t FindSNDX(const void *emi,
                           const uint32_t &type,
                           const uint64_t &flags = 0);

  static bool Is32(const void *emi);

  static bool Is64(const void *emi);

  static bool IsAmdGpu(const void *emi);

  static bool IsBigEndian(const void *emi);

  static bool IsLittleEndian(const void *emi);

  static bool IsValid(const void *emi);

  static uint64_t Size(const void *emi);

private:
  template<typename ElfN_Ehdr, typename ElfN_Shdr>
  static uint16_t FindSNDX(const void *emi,
                           const uint32_t &type,
                           const uint64_t &flags);

  template<typename ElfN_Ehdr>
  static bool IsAmdGpu(const void *emi);

  template<typename ElfN_Ehdr, typename ElfN_Shdr>
  static uint64_t Size(const void *emi);
};

//===----------------------------------------------------------------------===//
// AmdGpuImage.                                                               //
//===----------------------------------------------------------------------===//

class AmdGpuImage final {
public:
  static void DestroySymbols(void *amdgpumi);

  static hsa_status_t GetInfo(void *amdgpumi,
                              hsa_code_object_info_t attribute,
                              void *value);

  static hsa_status_t GetSymbol(void *amdgpumi,
                                const char *module_name,
                                const char *symbol_name,
                                hsa_code_symbol_t *sym_handle);

  static hsa_status_t IterateSymbols(hsa_code_object_t code_object,
                                     hsa_status_t (*callback)(
                                       hsa_code_object_t code_object,
                                       hsa_code_symbol_t symbol,
                                       void* data),
                                     void* data);

private:
  struct NDVersion {
    uint32_t major;
    uint32_t minor;
  };

  struct NDHsail {
    uint32_t major;
    uint32_t minor;
    uint8_t profile;
    uint8_t machine_model;
    uint8_t rounding_mode;
    uint8_t reserved0;
  };

  struct NDIsa {
    uint16_t vensz;
    uint16_t arcsz;
    uint32_t major;
    uint32_t minor;
    uint32_t stepping;
    char ven[4];
    char arc[6];
    char reserved[2];
  };

  static std::unordered_map<void*, std::vector<Symbol*>*> symbols_;
};

//===----------------------------------------------------------------------===//
// Symbol.                                                                    //
//===----------------------------------------------------------------------===//

//<<<
typedef uint32_t symbol_attribute32_t;
//<<<

class Symbol {
public:
  static hsa_code_symbol_t CConvert(Symbol *sym);

  static Symbol* CConvert(hsa_code_symbol_t sym_handle);

  static hsa_executable_symbol_t EConvert(Symbol *sym);

  static Symbol* EConvert(hsa_executable_symbol_t sym_handle);

  virtual ~Symbol() {}

  bool IsKernel() const {
    return HSA_SYMBOL_KIND_KERNEL == kind;
  }
  bool IsVariable() const {
    return HSA_SYMBOL_KIND_VARIABLE == kind;
  }

  bool is_loaded;
  hsa_symbol_kind_t kind;
  std::string name;
  hsa_symbol_linkage_t linkage;
  bool is_definition;
  uint64_t base;
  uint64_t address;
  hsa_agent_t agent;

protected:
  Symbol(const bool &_is_loaded,
         const hsa_symbol_kind_t &_kind,
         const std::string &_name,
         const hsa_symbol_linkage_t &_linkage,
         const bool &_is_definition,
         const uint64_t &_base = 0,
         const uint64_t &_address = 0)
    : is_loaded(_is_loaded)
    , kind(_kind)
    , name(_name)
    , linkage(_linkage)
    , is_definition(_is_definition)
    , base(_base)
    , address(_address) {}

  virtual hsa_status_t GetInfo(const symbol_attribute32_t &attribute, void *value);

private:
  Symbol(const Symbol &s);
  Symbol& operator=(const Symbol &s);
};

//===----------------------------------------------------------------------===//
// KernelSymbol.                                                              //
//===----------------------------------------------------------------------===//

class KernelSymbol final: public Symbol {
public:
  KernelSymbol(const bool &_is_loaded,
               const std::string &_name,
               const hsa_symbol_linkage_t &_linkage,
               const bool &_is_definition,
               const uint32_t &_kernarg_segment_size,
               const uint32_t &_kernarg_segment_alignment,
               const uint32_t &_group_segment_size,
               const uint32_t &_private_segment_size,
               const bool &_is_dynamic_callstack,
               const uint64_t &_base = 0,
               const uint64_t &_address = 0)
    : Symbol(_is_loaded,
             HSA_SYMBOL_KIND_KERNEL,
             _name,
             _linkage,
             _is_definition,
             _base,
             _address)
    , kernarg_segment_size(_kernarg_segment_size)
    , kernarg_segment_alignment(_kernarg_segment_alignment)
    , group_segment_size(_group_segment_size)
    , private_segment_size(_private_segment_size)
    , is_dynamic_callstack(_is_dynamic_callstack) {}

  ~KernelSymbol() {}

  hsa_status_t GetInfo(const symbol_attribute32_t &attribute, void *value);

  uint32_t kernarg_segment_size;
  uint32_t kernarg_segment_alignment;
  uint32_t group_segment_size;
  uint32_t private_segment_size;
  bool is_dynamic_callstack;

private:
  KernelSymbol(const KernelSymbol &ks);
  KernelSymbol& operator=(const KernelSymbol &ks);
};

//===----------------------------------------------------------------------===//
// VariableSymbol.                                                            //
//===----------------------------------------------------------------------===//

class VariableSymbol final: public Symbol {
public:
  VariableSymbol(const bool &_is_loaded,
                 const std::string &_name,
                 const hsa_symbol_linkage_t &_linkage,
                 const bool &_is_definition,
                 const hsa_variable_allocation_t &_allocation,
                 const hsa_variable_segment_t &_segment,
                 const uint32_t &_size,
                 const uint32_t &_alignment,
                 const bool &_is_constant,
                 const bool &_is_external = false,
                 const uint64_t &_base = 0,
                 const uint64_t &_address = 0)
    : Symbol(_is_loaded,
             HSA_SYMBOL_KIND_VARIABLE,
             _name,
             _linkage,
             _is_definition,
             _base,
             _address)
    , allocation(_allocation)
    , segment(_segment)
    , size(_size)
    , alignment(_alignment)
    , is_constant(_is_constant)
    , is_external(_is_external) {}

  ~VariableSymbol() {}

  hsa_status_t GetInfo(const symbol_attribute32_t &attribute, void *value);

  hsa_variable_allocation_t allocation;
  hsa_variable_segment_t segment;
  uint32_t size;
  uint32_t alignment;
  bool is_constant;
  bool is_external;

private:
  VariableSymbol(const VariableSymbol &vs);
  VariableSymbol& operator=(const VariableSymbol &vs);
};

//===----------------------------------------------------------------------===//
// Executable.                                                                //
//===----------------------------------------------------------------------===//

//<<<
class MemoryAddress final {
public:
  MemoryAddress(const uint64_t &_addr,
                const uint64_t &_p_vaddr,
                const uint64_t &_size,
                const hsa_agent_t &_agent)
    : addr(_addr)
    , p_vaddr(_p_vaddr)
    , size(_size)
    , agent(_agent) {}

  uint64_t addr;
  uint64_t p_vaddr;
  uint64_t size;
  hsa_agent_t agent;
};
//<<<

//<<<
typedef std::string ProgramSymbol;
typedef std::unordered_map<ProgramSymbol, Symbol*> ProgramSymbolMap;
//<<<

//<<<
typedef std::pair<std::string, hsa_agent_t> AgentSymbol;
struct ASC final {
  bool operator()(const AgentSymbol &las, const AgentSymbol &ras) const {
    return las.first == ras.first && las.second.handle == ras.second.handle;
  }
};
struct ASH final {
  size_t operator()(const AgentSymbol &as) const {
    size_t h = std::hash<std::string>()(as.first);
    size_t i = std::hash<uint64_t>()(as.second.handle);
    return h ^ (i << 1);
  }
};
typedef std::unordered_map<AgentSymbol, Symbol*, ASH, ASC> AgentSymbolMap;
//<<<

//<<<
class Sampler final {
public:
  Sampler(const hsa_agent_t &_owner,
          const hsa_ext_sampler_t &_handle)
    : owner(_owner)
    , handle(_handle) {}

  hsa_agent_t owner;
  hsa_ext_sampler_t handle;
};
//<<<

//<<<
class Image final {
public:
  Image(const hsa_agent_t &_owner,
        const hsa_ext_image_t &_handle)
    : owner(_owner)
    , handle(_handle) {}

  hsa_agent_t owner;
  hsa_ext_image_t handle;
};
//<<<
class DebugInfo final {
public:
  DebugInfo(void *_elf_raw, const size_t &_elf_size)
    : elf_raw(_elf_raw)
    , elf_size(_elf_size) {}

  void *elf_raw;
  size_t elf_size;
};
//<<<

class Executable final {
public:
  static hsa_executable_t Convert(Executable *exec);

  static Executable* Convert(hsa_executable_t exec_handle);

  const hsa_profile_t& profile() const {
    return profile_;
  }
  const hsa_executable_state_t& state() const {
    return state_;
  }

  Executable(const hsa_profile_t &_profile,
             const hsa_executable_state_t &_state);

  ~Executable();

  hsa_status_t Freeze() {
    if (HSA_EXECUTABLE_STATE_FROZEN == state_) {
      return HSA_STATUS_ERROR_FROZEN_EXECUTABLE;
    }

    state_ = HSA_EXECUTABLE_STATE_FROZEN;
    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t DefineProgramVariable(const char *var_name,
                                     void *address);

  hsa_status_t DefineAgentVariable(const char *var_name,
                                   const hsa_agent_t &agent,
                                   const hsa_variable_segment_t &segment,
                                   void *address);

  hsa_status_t GetSymbol(const char *module_name,
                         const char *symbol_name,
                         const hsa_agent_t &agent,
                         const int32_t &call_convention,
                         hsa_executable_symbol_t *symbol);

  hsa_status_t IterateSymbols(hsa_status_t (*callback)(
                                hsa_executable_t executable,
                                hsa_executable_symbol_t symbol,
                                void* data),
                              void* data);

  hsa_status_t LoadCodeObject(const hsa_agent_t &agent,
                              const hsa_code_object_t &code_object,
                              const char *options);

  hsa_status_t Validate(uint32_t *result);

private:
  Executable(const Executable &e);
  Executable& operator=(const Executable &e);

  size_t Alsec2AlsegNdx(const uint32_t &type, const uint64_t &flags);

  hsa_status_t AlsegCreate(const size_t &alseg_ndx,
                           const uint64_t &p_vaddr,
                           const hsa_agent_t &agent,
                           const uint64_t &size,
                           const uint64_t &align,
                           bool &new_prog_region);

  hsa_profile_t profile_;
  hsa_executable_state_t state_;

  std::array<std::list<MemoryAddress*>*, ALSEG_CNT_MAX> alsegs_;
  ProgramSymbolMap program_symbols_;
  AgentSymbolMap agent_symbols_;
  std::list<Sampler> samplers_;
  std::list<Image> images_;
  std::list<DebugInfo*> debug_info_;
};

} // namespace loader
} // namespace core

#endif // HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_
