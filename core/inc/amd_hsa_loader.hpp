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

#ifndef AMD_HSA_LOADER_HPP
#define AMD_HSA_LOADER_HPP

#include <cstddef>
#include <cstdint>
#include "hsa.h"
#include "hsa_ext_image.h"
#include "amd_hsa_elf.h"
#include "amd_load_map.h"
#include <string>
#include <mutex>
#include <vector>

/// @brief Major version of the AMD HSA Loader. Major versions are not backwards
/// compatible.
#define AMD_HSA_LOADER_VERSION_MAJOR 0

/// @brief Minor version of the AMD HSA Loader. Minor versions are backwards
/// compatible.
#define AMD_HSA_LOADER_VERSION_MINOR 5

/// @brief Descriptive version of the AMD HSA Loader.
#define AMD_HSA_LOADER_VERSION "AMD HSA Loader v0.05 (June 16, 2015)"

enum hsa_ext_symbol_info_t {
  HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_SIZE = 100,
  HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_ALIGN = 101,
};

typedef uint32_t hsa_symbol_info32_t;
typedef hsa_executable_symbol_t hsa_symbol_t;
typedef hsa_executable_symbol_info_t hsa_symbol_info_t;

namespace amd {
namespace hsa {
namespace loader {

//===----------------------------------------------------------------------===//
// Context.                                                                   //
//===----------------------------------------------------------------------===//

class Context {
public:
  virtual ~Context() {}

  virtual hsa_isa_t IsaFromName(const char *name) = 0;

  virtual bool IsaSupportedByAgent(hsa_agent_t agent, hsa_isa_t isa) = 0;

  virtual void* SegmentAlloc(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, size_t size, size_t align, bool zero) = 0;

  virtual bool SegmentCopy(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* dst, size_t offset, const void* src, size_t size) = 0;

  virtual void SegmentFree(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t size) = 0;

  virtual void* SegmentAddress(amdgpu_hsa_elf_segment_t segment, hsa_agent_t agent, void* seg, size_t offset) = 0;
  
  virtual bool ImageExtensionSupported() = 0;

  virtual hsa_status_t ImageCreate(
    hsa_agent_t agent,
    hsa_access_permission_t image_permission,
    const hsa_ext_image_descriptor_t *image_descriptor,
    const void *image_data,
    hsa_ext_image_t *image_handle) = 0;

  virtual hsa_status_t ImageDestroy(
    hsa_agent_t agent, hsa_ext_image_t image_handle) = 0;

  virtual hsa_status_t SamplerCreate(
    hsa_agent_t agent,
    const hsa_ext_sampler_descriptor_t *sampler_descriptor,
    hsa_ext_sampler_t *sampler_handle) = 0;

  virtual hsa_status_t SamplerDestroy(
    hsa_agent_t agent, hsa_ext_sampler_t sampler_handle) = 0;

protected:
  Context() {}

private:
  Context(const Context &c);
  Context& operator=(const Context &c);
};

//===----------------------------------------------------------------------===//
// Symbol.                                                                    //
//===----------------------------------------------------------------------===//

class Symbol {
public:
  static hsa_symbol_t Handle(Symbol *symbol) {
    hsa_symbol_t symbol_handle =
      {reinterpret_cast<uint64_t>(symbol)};
    return symbol_handle;
  }

  static Symbol* Object(hsa_symbol_t symbol_handle) {
    Symbol *symbol =
      reinterpret_cast<Symbol*>(symbol_handle.handle);
    return symbol;
  }

  virtual ~Symbol() {}

  virtual bool GetInfo(hsa_symbol_info32_t symbol_info, void *value) = 0;

protected:
  Symbol() {}

private:
  Symbol(const Symbol &s);
  Symbol& operator=(const Symbol &s);
};

//===----------------------------------------------------------------------===//
// LoadedCodeObject.                                                          //
//===----------------------------------------------------------------------===//

class LoadedCodeObject {
public:
  static amd_loaded_code_object_t Handle(LoadedCodeObject *object) {
    amd_loaded_code_object_t handle =
      {reinterpret_cast<uint64_t>(object)};
    return handle;
  }

  static LoadedCodeObject* Object(amd_loaded_code_object_t handle) {
    LoadedCodeObject *object =
      reinterpret_cast<LoadedCodeObject*>(handle.handle);
    return object;
  }

  virtual ~LoadedCodeObject() {}

  virtual bool GetInfo(amd_loaded_code_object_info_t attribute, void *value) = 0;

  virtual hsa_status_t IterateLoadedSegments(
    hsa_status_t (*callback)(
      amd_loaded_segment_t loaded_segment,
      void *data),
    void *data) = 0;

protected:
  LoadedCodeObject() {}

private:
  LoadedCodeObject(const LoadedCodeObject&);
  LoadedCodeObject& operator=(const LoadedCodeObject&);
};

//===----------------------------------------------------------------------===//
// LoadedSegment.                                                             //
//===----------------------------------------------------------------------===//

class LoadedSegment {
public:
  static amd_loaded_segment_t Handle(LoadedSegment *object) {
    amd_loaded_segment_t handle =
      {reinterpret_cast<uint64_t>(object)};
    return handle;
  }

  static LoadedSegment* Object(amd_loaded_segment_t handle) {
    LoadedSegment *object =
      reinterpret_cast<LoadedSegment*>(handle.handle);
    return object;
  }

  virtual ~LoadedSegment() {}

  virtual bool GetInfo(amd_loaded_segment_info_t attribute, void *value) = 0;

protected:
  LoadedSegment() {}

private:
  LoadedSegment(const LoadedSegment&);
  LoadedSegment& operator=(const LoadedSegment&);
};

//===----------------------------------------------------------------------===//
// Executable.                                                                //
//===----------------------------------------------------------------------===//

class Executable {
public:
  static hsa_executable_t Handle(Executable *executable) {
    hsa_executable_t executable_handle =
      {reinterpret_cast<uint64_t>(executable)};
    return executable_handle;
  }

  static Executable* Object(hsa_executable_t executable_handle) {
    Executable *executable =
      reinterpret_cast<Executable*>(executable_handle.handle);
    return executable;
  }

  static Executable* Create(
    hsa_profile_t profile, Context *context, const char *options);

  static void Destroy(Executable *executable);

  static hsa_status_t IterateExecutables(
    hsa_status_t (*callback)(
      hsa_executable_t executable,
      void *data),
    void *data);

  virtual ~Executable() {}

  virtual hsa_status_t GetInfo(
    hsa_executable_info_t executable_info, void *value) = 0;

  virtual hsa_status_t DefineProgramExternalVariable(
    const char *name, void *address) = 0;

  virtual hsa_status_t DefineAgentExternalVariable(
    const char *name,
    hsa_agent_t agent,
    hsa_variable_segment_t segment,
    void *address) = 0;

  virtual hsa_status_t LoadCodeObject(
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    const char *options,
    amd_loaded_code_object_t *loaded_code_object = nullptr) = 0;

  virtual hsa_status_t LoadCodeObject(
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    size_t code_object_size,
    const char *options,
    amd_loaded_code_object_t *loaded_code_object = nullptr) = 0;

  virtual hsa_status_t Freeze(const char *options) = 0;

  virtual hsa_status_t Validate(uint32_t *result) = 0;

  virtual Symbol* GetSymbol(
    const char *module_name,
    const char *symbol_name,
    hsa_agent_t agent,
    int32_t call_convention) = 0;

  typedef hsa_status_t (*iterate_symbols_f)(
    hsa_executable_t executable,
    hsa_symbol_t symbol_handle,
    void *data);

  virtual hsa_status_t IterateSymbols(
    iterate_symbols_f callback, void *data) = 0;

  virtual hsa_status_t IterateLoadedCodeObjects(
    hsa_status_t (*callback)(
      amd_loaded_code_object_t loaded_code_object,
      void *data),
    void *data) = 0;

protected:
  Executable() {}

private:
  Executable(const Executable &e);
  Executable& operator=(const Executable &e);

  static std::vector<Executable*> executables;
  static std::mutex executables_mutex;
};

} // namespace loader
} // namespace hsa
} // namespace amd

#endif // AMD_HSA_LOADER_HPP
