/******************************************************************************
* University of Illinois / NCSA
* Open Source License
*
* Copyright(c) 2011 - 2015  Advanced Micro Devices, Inc.
* All rights reserved.
*
* Developed by:
* Advanced Micro Devices, Inc.
* www.amd.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* with the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and /
* or sell copies of the Software, and to permit persons to whom the Software
* is furnished to do so, subject to the following conditions:
*
*     Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimers.
*
*     Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimers in the documentation
* and / or other materials provided with the distribution.
*
*     Neither the names of Advanced Micro Devices, Inc, nor the
mes of its
* contributors may be used to endorse or promote products derived from this
* Software without specific prior written permission.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
* THE SOFTWARE.
******************************************************************************/

#ifndef AMD_HSA_CODE_HPP_
#define AMD_HSA_CODE_HPP_

#include "amd_elf_image.hpp"
#include "amd_hsa_elf.h"
#include "amd_hsa_kernel_code.h"
#include "hsa.h"
#include <memory>
#include <sstream>
#include <cassert>
#include <unordered_map>

namespace amd {
namespace hsa {
namespace common {

template<uint64_t signature>
class Signed {
public:
  static const uint64_t CT_SIGNATURE;
  const uint64_t RT_SIGNATURE;

protected:
  Signed(): RT_SIGNATURE(signature) {}
  virtual ~Signed() {}
};

template<uint64_t signature>
const uint64_t Signed<signature>::CT_SIGNATURE = signature;

bool IsAccessibleMemoryAddress(uint64_t address);

template<typename class_type, typename member_type>
size_t OffsetOf(member_type class_type::*member)
{
  return (char*)&((class_type*)nullptr->*member) - (char*)nullptr;
}

template<typename class_type>
class_type* ObjectAt(uint64_t address)
{
  if (!IsAccessibleMemoryAddress(address)) {
    return nullptr;
  }

  const uint64_t *rt_signature =
    (const uint64_t*)(address + OffsetOf(&class_type::RT_SIGNATURE));
  if (nullptr == rt_signature) {
    return nullptr;
  }
  if (class_type::CT_SIGNATURE != *rt_signature) {
    return nullptr;
  }

  return (class_type*)address;
}

}

namespace code {

    typedef amd::elf::Segment Segment;
    typedef amd::elf::Section Section;
    typedef amd::elf::RelocationSection RelocationSection;
    typedef amd::elf::Relocation Relocation;

    class KernelSymbol;
    class VariableSymbol;

    class Symbol {
    protected:
      amd::elf::Symbol* elfsym;

    public:
      explicit Symbol(amd::elf::Symbol* elfsym_)
        : elfsym(elfsym_) { }
      virtual ~Symbol() { }
      virtual bool IsKernelSymbol() const { return false; }
      virtual KernelSymbol* AsKernelSymbol() { assert(false); return 0; }
      virtual bool IsVariableSymbol() const { return false; }
      virtual VariableSymbol* AsVariableSymbol() { assert(false); return 0; }
      std::string Name() const { return elfsym->name(); }
      Section* GetSection() { return elfsym->section(); }
      uint64_t SectionOffset() const { return elfsym->value(); }
      uint64_t VAddr() const { return elfsym->section()->addr() + elfsym->value(); }
      uint32_t Index() const { return elfsym->index(); }
      bool IsDeclaration() const;
      bool IsDefinition() const;
      bool IsAgent() const;
      virtual hsa_symbol_kind_t Kind() const = 0;
      hsa_symbol_linkage_t Linkage() const;
      hsa_variable_allocation_t Allocation() const;
      hsa_variable_segment_t Segment() const;
      uint32_t Size() const;
      uint32_t Alignment() const;
      bool IsConst() const;
      virtual hsa_status_t GetInfo(hsa_code_symbol_info_t attribute, void *value);
      static hsa_code_symbol_t ToHandle(Symbol* sym);
      static Symbol* FromHandle(hsa_code_symbol_t handle);
    };

    class KernelSymbol : public Symbol {
    private:
      uint32_t kernarg_segment_size, kernarg_segment_alignment;
      uint32_t group_segment_size, private_segment_size;
      bool is_dynamic_callstack;

    public:
      explicit KernelSymbol(amd::elf::Symbol* elfsym_, const amd_kernel_code_t* akc);
      bool IsKernelSymbol() const override { return true; }
      KernelSymbol* AsKernelSymbol() override { return this; }
      hsa_symbol_kind_t Kind() const override { return HSA_SYMBOL_KIND_KERNEL; }
      hsa_status_t GetInfo(hsa_code_symbol_info_t attribute, void *value) override;
    };

    class VariableSymbol : public Symbol {
    public:
      explicit VariableSymbol(amd::elf::Symbol* elfsym_)
        : Symbol(elfsym_) { }
      bool IsVariableSymbol() const override { return true; }
      VariableSymbol* AsVariableSymbol() override { return this; }
      hsa_symbol_kind_t Kind() const override { return HSA_SYMBOL_KIND_VARIABLE; }
      hsa_status_t GetInfo(hsa_code_symbol_info_t attribute, void *value) override;
    };

    class AmdHsaCode {
    private:
      std::ostringstream out;
      std::unique_ptr<amd::elf::Image> img;
      std::vector<Segment*> dataSegments;
      std::vector<Section*> dataSections;
      std::vector<RelocationSection*> relocationSections;
      std::vector<Symbol*> symbols;
      bool owned;
      bool combineDataSegments;

      amd::elf::Section* hsatext;
      amd::elf::Section* imageInit;
      amd::elf::Section* samplerInit;

      bool PullElf();

      void AddAmdNote(uint32_t type, const void* desc, uint32_t desc_size);
      template <typename S>
      bool GetAmdNote(uint32_t type, S** desc)
      {
        uint32_t desc_size;
        if (!img->note()->getNote("AMD", type, (void**) desc, &desc_size)) {
          out << "Failed to find note, type: " << type << std::endl;
          return false;
        }
        if (desc_size < sizeof(S)) {
          out << "Note size mismatch, type: " << type << " size: " << desc_size << " expected at least " << sizeof(S) << std::endl;
          return false;
        }
        return true;
      }

      amd::elf::Segment* MachineCode();
      amd::elf::Section* HsaText();
      amd::elf::Section* ImageInit();
      amd::elf::Section* SamplerInit();

      void PrintSegment(std::ostream& out, Segment* segment);
      void PrintSection(std::ostream& out, Section* section);
      void PrintRawData(std::ostream& out, Section* section);
      void PrintRawData(std::ostream& out, const unsigned char *data, size_t size);
      void PrintRelocationData(std::ostream& out, RelocationSection* section);
      void PrintSymbol(std::ostream& out, Symbol* sym);
      void PrintMachineCode(std::ostream& out, KernelSymbol* sym);
      void PrintDisassembly(std::ostream& out, const unsigned char *isa, size_t size);
      std::string MangleSymbolName(const std::string& module_name, const std::string symbol_name);

    public:
      AmdHsaCode(amd::elf::Image* img_ = 0, bool combineDataSegments = true);
      ~AmdHsaCode();

      std::string output() { return out.str(); }
      bool LoadFromFile(const std::string& filename);
      bool SaveToFile(const std::string& filename);
      bool InitFromBuffer(const void* buffer, size_t size);
      bool InitAsBuffer(const void* buffer, size_t size);
      bool InitAsHandle(hsa_code_object_t code_handle);
      hsa_code_object_t GetHandle();
      const char* ElfData();
      uint64_t ElfSize();
      bool Validate();
      void Print(std::ostream& out);
      void PrintNotes(std::ostream& out);
      void PrintSegments(std::ostream& out);
      void PrintSections(std::ostream& out);
      void PrintSymbols(std::ostream& out);
      void PrintMachineCode(std::ostream& out);
      bool PrintToFile(const std::string& filename);

      void AddNoteCodeObjectVersion(uint32_t major, uint32_t minor);
      bool GetNoteCodeObjectVersion(uint32_t* major, uint32_t* minor);
      bool GetNoteCodeObjectVersion(std::string& version);
      void AddNoteHsail(uint32_t hsail_major, uint32_t hsail_minor, hsa_profile_t profile, hsa_machine_model_t machine_model, hsa_default_float_rounding_mode_t rounding_mode);
      bool GetNoteHsail(uint32_t* hsail_major, uint32_t* hsail_minor, hsa_profile_t* profile, hsa_machine_model_t* machine_model, hsa_default_float_rounding_mode_t* default_float_round);
      void AddNoteIsa(const std::string& vendor_name, const std::string& architecture_name, uint32_t major, uint32_t minor, uint32_t stepping);
      bool GetNoteIsa(std::string& vendor_name, std::string& architecture_name, uint32_t* major_version, uint32_t* minor_version, uint32_t* stepping);
      bool GetNoteIsa(std::string& isaName);
      void AddNoteProducer(uint32_t major, uint32_t minor, const std::string& producer);
      bool GetNoteProducer(uint32_t* major, uint32_t* minor, std::string& producer_name);
      void AddNoteProducerOptions(const std::string& options);
      bool GetNoteProducerOptions(std::string& options);

      hsa_status_t GetInfo(hsa_code_object_info_t attribute, void *value);
      hsa_status_t GetSymbol(const char *module_name, const char *symbol_name, hsa_code_symbol_t *sym);
      hsa_status_t IterateSymbols(hsa_code_object_t code_object,
                                  hsa_status_t (*callback)(
                                    hsa_code_object_t code_object,
                                    hsa_code_symbol_t symbol,
                                    void* data),
                                  void* data);

      void AddHsaTextData(const void* buffer, size_t size);

      Symbol* AddKernelDefinition(const std::string& name, const void* isa, size_t isa_size);

      size_t DataSegmentCount() { return dataSegments.size(); }
      Segment* DataSegment(size_t i) { return dataSegments[i]; }

      size_t DataSectionCount() { return dataSections.size(); }
      Section* DataSection(size_t i) { return dataSections[i]; }

      size_t RelocationSectionCount() { return relocationSections.size(); }
      RelocationSection* GetRelocationSection(size_t i) { return relocationSections[i]; }

      size_t SymbolCount() { return symbols.size(); }
      Symbol* GetSymbol(size_t i) { return symbols[i]; }

      void AddData(amdgpu_hsa_elf_section_t section, const void* data = 0, size_t size = 0);

      void AddImageInit(
        amdgpu_hsa_metadata_kind16_t kind,
        amdgpu_hsa_image_geometry8_t geometry,
        amdgpu_hsa_image_channel_order8_t channel_order, amdgpu_hsa_image_channel_type8_t channel_type,
        uint64_t width, uint64_t height, uint64_t depth, uint64_t array);
    };

    class AmdHsaCodeManager {
    private:
      typedef std::unordered_map<uint64_t, AmdHsaCode*> CodeMap;
      CodeMap codeMap;

    public:
      AmdHsaCode* FromHandle(hsa_code_object_t handle);
      bool Destroy(hsa_code_object_t handle);
    };
}
}
}

#endif // AMD_HSA_CODE_HPP_
