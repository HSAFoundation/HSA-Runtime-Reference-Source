#ifndef HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_
#define HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_

#include <string>
#include <unordered_map>
#include <utility>
#include "core/inc/hsa_internal.h"
#include <libelf.h>
#include <list>

namespace core {
namespace loader {

struct CodeObject {
  void *elf;
  uint64_t size;
  bool globals;
  bool loaded;
  Elf *ed;
};

hsa_status_t SerializeCodeObject(
  hsa_code_object_t code_object,
  hsa_status_t (*alloc_callback)(
    size_t size, hsa_callback_data_t data, void **address
  ),
  hsa_callback_data_t callback_data,
  const char *options,
  void **serialized_code_object,
  size_t *serialized_code_object_size
);

hsa_status_t DeserializeCodeObject(
  void *serialized_code_object,
  size_t serialized_code_object_size,
  const char *options,
  hsa_code_object_t *code_object
);

//
hsa_status_t DestroyCodeObject(hsa_code_object_t code_object);

//
hsa_status_t GetCodeObjectInfo(
  hsa_code_object_t code_object,
  hsa_code_object_info_t attribute,
  void *value
);

//
hsa_status_t GetCodeObjectSymbol(
  hsa_code_object_t code_object,
  const char *symbol_name,
  hsa_code_symbol_t *symbol
);

//
hsa_status_t GetCodeObjectSymbolInfo(
  hsa_code_symbol_t code_symbol,
  hsa_code_symbol_info_t attribute,
  void *value
);

// TODO
hsa_status_t IterateCodeObjectSymbols(
  hsa_code_object_t code_object,
  hsa_status_t (*callback)(
    hsa_code_object_t code_object, hsa_code_symbol_t symbol, void *data
  ),
  void *data
);

hsa_status_t GetSymbolInfo(
  hsa_executable_symbol_t executable_symbol,
  hsa_executable_symbol_info_t attribute,
  void *value
);

struct SymbolInfo {
  SymbolInfo(const uint64_t &addr, Elf *e, hsa_agent_t a,
    size_t ssndx):
    address(addr), elf(e), agent(a), sstrtabndx(ssndx) {}

  uint64_t address;
  Elf *elf;
  hsa_agent_t agent;
  size_t sstrtabndx;
};

struct KernelSymbol {
  KernelSymbol(
    void *in_code_object_image,
    size_t in_code_object_image_size,
    size_t in_symbol_index
  ):
    code_object_image(in_code_object_image),
    code_object_image_size(in_code_object_image_size),
    symbol_index(in_symbol_index) {}

  void *code_object_image;
  size_t code_object_image_size;
  size_t symbol_index;
};

class Executable final {
public:
  const hsa_profile_t& profile() const {
    return profile_;
  }
  const hsa_executable_state_t& executable_state() const {
    return executable_state_;
  }

  static hsa_status_t Create(
    hsa_profile_t profile,
    hsa_executable_state_t executable_state,
    const char *options,
    hsa_executable_t *executable
  );

  static hsa_status_t Destroy(hsa_executable_t executable);

  static hsa_executable_t Handle(const Executable *exec);

  static Executable* Object(const hsa_executable_t &executable);

  hsa_status_t Freeze(const char *options);

  hsa_status_t GetInfo(const hsa_executable_info_t &attribute, void *value);

  hsa_status_t GetSymbol(
    const char *module_name,
    const char *symbol_name,
    hsa_agent_t agent,
    int32_t call_convention,
    hsa_executable_symbol_t *symbol
  );

  hsa_status_t IterateSymbols(
    hsa_status_t (*callback)(
      hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data
    ),
    void *data
  );

  hsa_status_t LoadCodeObject(
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    const char *options
  );

private:
  Executable();

  Executable(
    const hsa_profile_t &profile,
    const hsa_executable_state_t &executable_state
  ) : profile_(profile), executable_state_(executable_state),
      code_base_(NULL), code_size_(0), agent_base_(0), prog_base_(0) {}

  Executable& operator=(const Executable &e);

  ~Executable();

  const hsa_profile_t profile_;
  hsa_executable_state_t executable_state_;
  std::unordered_map<std::string, std::pair<Elf64_Sym, SymbolInfo>> execs_;

  char *code_base_;
  size_t code_size_;

  uint64_t agent_base_;
  uint64_t prog_base_;

  std::list<KernelSymbol> kernels_;
}; // class Executable

} // namespace loader
} // namespace core

#endif // HSA_RUNTIME_CORE_LOADER_EXECUTABLE_HPP_
