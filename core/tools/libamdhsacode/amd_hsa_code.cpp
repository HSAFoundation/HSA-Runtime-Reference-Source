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

#include <assert.h>
#include <cstring>
#include <iomanip>
#include "amd_hsa_code.hpp"
#include "amd_hsa_code_util.hpp"
#include <libelf.h>
#include "amd_hsa_elf.h"
#include <fstream>


#ifndef _WIN32
#define _alloca alloca
#endif

namespace amd {
namespace hsa {
namespace code {

    using amd::elf::GetNoteString;

    bool Symbol::IsDeclaration() const
    {
      return elfsym->type() == STT_COMMON;
    }

    bool Symbol::IsDefinition() const
    {
      return !IsDeclaration();
    }

    bool Symbol::IsAgent() const
    {
      return elfsym->section()->flags() & SHF_AMDGPU_HSA_AGENT ? true : false;
    }

    hsa_symbol_linkage_t Symbol::Linkage() const
    {
      return elfsym->binding() == STB_GLOBAL ? HSA_SYMBOL_LINKAGE_PROGRAM : HSA_SYMBOL_LINKAGE_MODULE;
    }

    hsa_variable_allocation_t Symbol::Allocation() const
    {
      return IsAgent() ? HSA_VARIABLE_ALLOCATION_AGENT : HSA_VARIABLE_ALLOCATION_PROGRAM;
    }

    hsa_variable_segment_t Symbol::Segment() const
    {
      return elfsym->section()->flags() & SHF_AMDGPU_HSA_READONLY ? HSA_VARIABLE_SEGMENT_READONLY : HSA_VARIABLE_SEGMENT_GLOBAL;
    }

    uint32_t Symbol::Size() const
    {
      assert(elfsym->size() < UINT32_MAX);
      return uint32_t(elfsym->size());
    }

    uint32_t Symbol::Alignment() const
    {
      assert(elfsym->section()->addralign() < UINT32_MAX);
      return uint32_t(elfsym->section()->addralign());
    }

    bool Symbol::IsConst() const
    {
      return elfsym->section()->flags() & SHF_WRITE ? true : false;
    }

    hsa_status_t Symbol::GetInfo(hsa_code_symbol_info_t attribute, void *value)
    {
      assert(value);
      std::string name = Name();
      switch (attribute) {
        case HSA_CODE_SYMBOL_INFO_TYPE: {
          *((hsa_symbol_kind_t*)value) = Kind();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_NAME_LENGTH:
        case HSA_CODE_SYMBOL_INFO_NAME: {
          std::string matter = "";
          switch (Linkage()) {
          case HSA_SYMBOL_LINKAGE_PROGRAM:
            assert(name.rfind(":") == std::string::npos);
            matter = name;
            break;
          case HSA_SYMBOL_LINKAGE_MODULE:
            assert(name.rfind(":") != std::string::npos);
            matter = name.substr(name.rfind(":") + 1);
            break;
          default:
            assert(!"Unsupported linkage in Symbol::GetInfo");
            return HSA_STATUS_ERROR;
          }
          if (attribute == HSA_CODE_SYMBOL_INFO_NAME_LENGTH) {
            *((uint32_t*) value) = matter.size() + 1;
          } else {
            memset(value, 0x0, matter.size() + 1);
            memcpy(value, matter.c_str(), matter.size());
          }
          break;
        }
        case HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH:
        case HSA_CODE_SYMBOL_INFO_MODULE_NAME: {
          switch (Linkage()) {
          case HSA_SYMBOL_LINKAGE_PROGRAM:
            if (attribute == HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH) {
              *((uint32_t*) value) = 0;
            }
            break;
          case HSA_SYMBOL_LINKAGE_MODULE: {
            assert(name.find(":") != std::string::npos);
            std::string matter = name.substr(0, name.find(":"));
            if (attribute == HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH) {
              *((uint32_t*) value) = matter.length() + 1;
            } else {
              memset(value, 0x0, matter.size() + 1);
              memcpy(value, matter.c_str(), matter.length());
              ((char*)value)[matter.size() + 1] = '\0';
            }
            break;
          }
          default:
            assert(!"Unsupported linkage in Symbol::GetInfo");
            return HSA_STATUS_ERROR;
          }
          break;
        }
        case HSA_CODE_SYMBOL_INFO_LINKAGE: {
          *((hsa_symbol_linkage_t*)value) = Linkage();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_IS_DEFINITION: {
          *((bool*)value) = IsDefinition();
          break;
        }
        default: {
          return HSA_STATUS_ERROR_INVALID_ARGUMENT;
        }
      }
      return HSA_STATUS_SUCCESS;
    }

    hsa_code_symbol_t Symbol::ToHandle(Symbol* sym)
    {
      hsa_code_symbol_t s;
      s.handle = reinterpret_cast<uint64_t>(sym);
      return s;
    }

    Symbol* Symbol::FromHandle(hsa_code_symbol_t s)
    {
      return reinterpret_cast<Symbol*>(s.handle);
    }

    KernelSymbol::KernelSymbol(amd::elf::Symbol* elfsym_, const amd_kernel_code_t* akc)
        : Symbol(elfsym_)
    {
      kernarg_segment_size = (uint32_t) akc->kernarg_segment_byte_size;
      kernarg_segment_alignment = (uint32_t) (1 << akc->kernarg_segment_alignment);
      group_segment_size = uint32_t(akc->workgroup_group_segment_byte_size);
      private_segment_size = uint32_t(akc->workitem_private_segment_byte_size);
      is_dynamic_callstack =
        AMD_HSA_BITS_GET(akc->kernel_code_properties, AMD_KERNEL_CODE_PROPERTIES_IS_DYNAMIC_CALLSTACK) ? true : false;
    }

    hsa_status_t KernelSymbol::GetInfo(hsa_code_symbol_info_t attribute, void *value)
    {
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

    hsa_status_t VariableSymbol::GetInfo(hsa_code_symbol_info_t attribute, void *value)
    {
      assert(value);
      switch (attribute) {
        case HSA_CODE_SYMBOL_INFO_VARIABLE_ALLOCATION: {
          *((hsa_variable_allocation_t*)value) = Allocation();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_VARIABLE_SEGMENT: {
          *((hsa_variable_segment_t*)value) = Segment();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT: {
          *((uint32_t*)value) = Alignment();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_VARIABLE_SIZE: {
          *((uint32_t*)value) = Size();
          break;
        }
        case HSA_CODE_SYMBOL_INFO_VARIABLE_IS_CONST: {
          *((bool*)value) = IsConst();
          break;
        }
        default: {
          return Symbol::GetInfo(attribute, value);
        }
      }
      return HSA_STATUS_SUCCESS;
    }

    AmdHsaCode::AmdHsaCode(amd::elf::Image* img_, bool combineDataSegments_)
      : img(img_),
        owned(img_ != 0),
        combineDataSegments(combineDataSegments_),
        hsatext(0)
    {
    }

    AmdHsaCode::~AmdHsaCode()
    {
      for (Symbol* sym : symbols) { delete sym; }
      if (!owned) { img.release(); }
    }

    bool AmdHsaCode::PullElf()
    {
      for (size_t i = 0; i < img->segmentCount(); ++i) {
        Segment* s = img->segment(i);
        if (s->type() == PT_AMDGPU_HSA_LOAD_GLOBAL_PROGRAM ||
            s->type() == PT_AMDGPU_HSA_LOAD_GLOBAL_AGENT ||
            s->type() == PT_AMDGPU_HSA_LOAD_READONLY_AGENT ||
            s->type() == PT_AMDGPU_HSA_LOAD_CODE_AGENT) {
          dataSegments.push_back(s);
        }
      }
      for (size_t i = 0; i < img->sectionCount(); ++i) {
        Section* sec = img->section(i);
        if (!sec) { continue; }
        if ((sec->type() == SHT_PROGBITS || sec->type() == SHT_NOBITS) &&
            (sec->flags() & (SHF_AMDGPU_HSA_AGENT | SHF_AMDGPU_HSA_GLOBAL | SHF_AMDGPU_HSA_READONLY | SHF_AMDGPU_HSA_CODE))) {
          dataSections.push_back(sec);
        } else if (sec->type() == SHT_RELA) {
          relocationSections.push_back(sec->asRelocationSection());
        }
        if (sec->Name() == ".hsatext") {
          hsatext = sec;
        }
      }
      for (size_t i = 0; i < img->symtab()->symbolCount(); ++i) {
        amd::elf::Symbol* elfsym = img->symtab()->symbol(i);
        Symbol* sym = 0;
        switch (elfsym->type()) {
        case STT_AMDGPU_HSA_KERNEL: {
          amd_kernel_code_t akc;
          if (!hsatext->getData(elfsym->value(), &akc, sizeof(amd_kernel_code_t))) { return false; }
          sym = new KernelSymbol(elfsym, &akc);
          break;
        }
        case STT_OBJECT:
        case STT_COMMON:
          sym = new VariableSymbol(elfsym);
          break;
        default:
          break; // Skip unknown symbols.
        }
        if (sym) { symbols.push_back(sym); }
      }

      return true;
    }

    bool AmdHsaCode::LoadFromFile(const std::string& filename)
    {
      if (!img) { img.reset(amd::elf::NewElf64Image()); }
      if (!img->loadFromFile(filename)) {
        out << img->output();
        return false;
      }
      if (!PullElf()) { return false; }
      return true;
    }

    bool AmdHsaCode::SaveToFile(const std::string& filename)
    {
      return img->saveToFile(filename);
    }

    bool AmdHsaCode::InitFromBuffer(const void* buffer, size_t size)
    {
      if (!img) { img.reset(amd::elf::NewElf64Image()); }
      if (!img->initFromBuffer(buffer, size)) {
        out << img->output();
        return false;
      }
      if (!PullElf()) { return false; }
      return true;
    }

    bool AmdHsaCode::InitAsBuffer(const void* buffer, size_t size)
    {
      if (!img) { img.reset(amd::elf::NewElf64Image()); }
      if (!img->initAsBuffer(buffer, size)) {
        out << img->output();
        return false;
      }
      if (!PullElf()) { return false; }
      return true;
    }

    bool AmdHsaCode::InitAsHandle(hsa_code_object_t code_object)
    {
      void *elfmemrd = reinterpret_cast<void*>(code_object.handle);
      if (!elfmemrd) { return false; }
      return InitAsBuffer(elfmemrd, 0);
    }

    hsa_code_object_t AmdHsaCode::GetHandle()
    {
      hsa_code_object_t code_object;
      code_object.handle = reinterpret_cast<uint64_t>(img->data());
      return code_object;
    }

    const char* AmdHsaCode::ElfData()
    {
      return img->data();
    }

    uint64_t AmdHsaCode::ElfSize()
    {
      return img->size();
    }

    bool AmdHsaCode::Validate()
    {
      if (!img->Validate()) {
        out << img->output();
        return false;
      }
      if (img->Machine() != EM_AMDGPU) {
        out << "ELF error: Invalid machine" << std::endl;
        return false;
      }
      return true;
    }

    void AmdHsaCode::AddAmdNote(uint32_t type, const void* desc, uint32_t desc_size)
    {
      img->note()->addNote("AMD", type, desc, desc_size);
    }

    void AmdHsaCode::AddNoteCodeObjectVersion(uint32_t major, uint32_t minor)
    {
      amdgpu_hsa_note_code_object_version_t desc;
      desc.major_version = major;
      desc.minor_version = minor;
      AddAmdNote(NT_AMDGPU_HSA_CODE_OBJECT_VERSION, &desc, sizeof(desc));
    }

    bool AmdHsaCode::GetNoteCodeObjectVersion(uint32_t* major, uint32_t* minor)
    {
      amdgpu_hsa_note_code_object_version_t* desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_CODE_OBJECT_VERSION, &desc)) { return false; }
      *major = desc->major_version;
      *minor = desc->minor_version;
      return true;
    }

    bool AmdHsaCode::GetNoteCodeObjectVersion(std::string& version)
    {
      amdgpu_hsa_note_code_object_version_t* desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_CODE_OBJECT_VERSION, &desc)) { return false; }
      version.clear();
      version += std::to_string(desc->major_version);
      version += ".";
      version += std::to_string(desc->minor_version);
      return true;
    }

    void AmdHsaCode::AddNoteHsail(uint32_t hsail_major, uint32_t hsail_minor, hsa_profile_t profile, hsa_machine_model_t machine_model, hsa_default_float_rounding_mode_t rounding_mode)
    {
      amdgpu_hsa_note_hsail_t desc;
      memset(&desc, 0, sizeof(desc));
      desc.hsail_major_version = hsail_major;
      desc.hsail_minor_version = hsail_minor;
      desc.profile = uint8_t(profile);
      desc.machine_model = uint8_t(machine_model);
      desc.default_float_round = uint8_t(rounding_mode);
      AddAmdNote(NT_AMDGPU_HSA_HSAIL, &desc, sizeof(desc));
    }

    bool AmdHsaCode::GetNoteHsail(uint32_t* hsail_major, uint32_t* hsail_minor, hsa_profile_t* profile, hsa_machine_model_t* machine_model, hsa_default_float_rounding_mode_t* default_float_round)
    {
      amdgpu_hsa_note_hsail_t *desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_HSAIL, &desc)) { return false; }
      *hsail_major = desc->hsail_major_version;
      *hsail_minor = desc->hsail_minor_version;
      *profile = (hsa_profile_t) desc->profile;
      *machine_model = (hsa_machine_model_t) desc->machine_model;
      *default_float_round = (hsa_default_float_rounding_mode_t) desc->default_float_round;
      return true;
    }

    void AmdHsaCode::AddNoteIsa(const std::string& vendor_name, const std::string& architecture_name, uint32_t major, uint32_t minor, uint32_t stepping)
    {
      size_t size = sizeof(amdgpu_hsa_note_producer_t) + vendor_name.length() + architecture_name.length() + 1;
      amdgpu_hsa_note_isa_t* desc = (amdgpu_hsa_note_isa_t*) _alloca(size);
      memset(desc, 0, size);
      desc->vendor_name_size = vendor_name.length()+1;
      desc->architecture_name_size = architecture_name.length()+1;
      desc->major = major;
      desc->minor = minor;
      desc->stepping = stepping;
      memcpy(desc->vendor_and_architecture_name, vendor_name.c_str(), vendor_name.length() + 1);
      memcpy(desc->vendor_and_architecture_name + desc->vendor_name_size, architecture_name.c_str(), architecture_name.length() + 1);
      AddAmdNote(NT_AMDGPU_HSA_ISA, &desc, sizeof(desc));
    }

    bool AmdHsaCode::GetNoteIsa(std::string& vendor_name, std::string& architecture_name, uint32_t* major_version, uint32_t* minor_version, uint32_t* stepping)
    {
      amdgpu_hsa_note_isa_t *desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_ISA, &desc)) { return false; }
      vendor_name = GetNoteString(desc->vendor_name_size, desc->vendor_and_architecture_name);
      architecture_name = GetNoteString(desc->architecture_name_size, desc->vendor_and_architecture_name + vendor_name.length() + 1);
      *major_version = desc->major;
      *minor_version = desc->minor;
      *stepping = desc->stepping;
      return true;
    }

    bool AmdHsaCode::GetNoteIsa(std::string& isaName)
    {
      std::string vendor_name, architecture_name;
      uint32_t major_version, minor_version, stepping;
      if (!GetNoteIsa(vendor_name, architecture_name, &major_version, &minor_version, &stepping)) { return false; }
      isaName.clear();
      isaName += vendor_name;
      isaName += ":";
      isaName += architecture_name;
      isaName += ":";
      isaName += std::to_string(major_version);
      isaName += ":";
      isaName += std::to_string(minor_version);
      isaName += ":";
      isaName += std::to_string(stepping);
      return true;
    }

    void AmdHsaCode::AddNoteProducer(uint32_t major, uint32_t minor, const std::string& producer)
    {
      size_t size = sizeof(amdgpu_hsa_note_producer_t) + producer.length();
      amdgpu_hsa_note_producer_t* desc = (amdgpu_hsa_note_producer_t*) _alloca(size);
      memset(desc, 0, size);
      desc->producer_name_size = producer.length();
      desc->producer_major_version = major;
      desc->producer_minor_version = minor;
      memcpy(desc->producer_name, producer.c_str(), producer.length() + 1);
      AddAmdNote(NT_AMDGPU_HSA_PRODUCER, &desc, size);
    }

    bool AmdHsaCode::GetNoteProducer(uint32_t* major, uint32_t* minor, std::string& producer_name)
    {
      amdgpu_hsa_note_producer_t* desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_PRODUCER, &desc)) { return false; }
      *major = desc->producer_major_version;
      *minor = desc->producer_minor_version;
      producer_name = GetNoteString(desc->producer_name_size, desc->producer_name);
      return true;
    }

    void AmdHsaCode::AddNoteProducerOptions(const std::string& options)
    {
      size_t size = sizeof(amdgpu_hsa_note_producer_options_t) + options.length();
      amdgpu_hsa_note_producer_options_t *desc = (amdgpu_hsa_note_producer_options_t*) _alloca(size);
      memcpy(desc->producer_options, options.c_str(), options.length() + 1);
      AddAmdNote(NT_AMDGPU_HSA_PRODUCER_OPTIONS, desc, size);
    }

    bool AmdHsaCode::GetNoteProducerOptions(std::string& options)
    {
      amdgpu_hsa_note_producer_options_t* desc;
      if (!GetAmdNote(NT_AMDGPU_HSA_PRODUCER_OPTIONS, &desc)) { return false; }
      options = GetNoteString(desc->producer_options_size, desc->producer_options);
      return true;
    }

    hsa_status_t AmdHsaCode::GetInfo(hsa_code_object_info_t attribute, void *value)
    {
      assert(value);
      switch (attribute) {
      case HSA_CODE_OBJECT_INFO_VERSION: {
        std::string version;
        if (!GetNoteCodeObjectVersion(version)) { return HSA_STATUS_ERROR_INVALID_CODE_OBJECT; }
        char *svalue = (char*)value;
        memset(svalue, 0x0, 64);
        memcpy(svalue, version.c_str(), (std::min)(size_t(63), version.length()));
        break;
      }
      case HSA_CODE_OBJECT_INFO_ISA: {
        // TODO: Currently returns string representation instead of hsa_isa_t
        // which is unavailable here.
        std::string isa;
        if (!GetNoteIsa(isa)) { return HSA_STATUS_ERROR_INVALID_CODE_OBJECT; }
        char *svalue = (char*)value;
        memset(svalue, 0x0, 64);
        memcpy(svalue, isa.c_str(), (std::min)(size_t(63), isa.length()));
        break;
      }
      case HSA_CODE_OBJECT_INFO_MACHINE_MODEL:
      case HSA_CODE_OBJECT_INFO_PROFILE:
      case HSA_CODE_OBJECT_INFO_DEFAULT_FLOAT_ROUNDING_MODE: {
        uint32_t hsail_major, hsail_minor;
        hsa_profile_t profile;
        hsa_machine_model_t machine_model;
        hsa_default_float_rounding_mode_t default_float_round;
        if (!GetNoteHsail(&hsail_major, &hsail_minor, &profile, &machine_model, &default_float_round)) {
          return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
        }
        switch (attribute) {
        case HSA_CODE_OBJECT_INFO_MACHINE_MODEL:
           *((hsa_machine_model_t*)value) = machine_model; break;
        case HSA_CODE_OBJECT_INFO_PROFILE:
          *((hsa_profile_t*)value) = profile; break;
        case HSA_CODE_OBJECT_INFO_DEFAULT_FLOAT_ROUNDING_MODE:
          *((hsa_default_float_rounding_mode_t*)value) = default_float_round; break;
        default: break;
        }
        break;
      }
      default:
        assert(false);
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
      }
      return HSA_STATUS_SUCCESS;
    }

    hsa_status_t AmdHsaCode::GetSymbol(const char *module_name, const char *symbol_name, hsa_code_symbol_t *s)
    {
      std::string mname = MangleSymbolName(module_name ? module_name : "", symbol_name);
      for (Symbol* sym : symbols) {
        if (sym->Name() == mname) {
          *s = Symbol::ToHandle(sym);
          return HSA_STATUS_SUCCESS;
        }
      }
      return HSA_STATUS_ERROR_INVALID_SYMBOL_NAME;
    }

    hsa_status_t AmdHsaCode::IterateSymbols(hsa_code_object_t code_object,
                                  hsa_status_t (*callback)(
                                  hsa_code_object_t code_object,
                                  hsa_code_symbol_t symbol,
                                  void* data),
                                void* data)
    {
      for (Symbol* sym : symbols) {
        hsa_code_symbol_t s = Symbol::ToHandle(sym);
        hsa_status_t status = callback(code_object, s, data);
        if (status != HSA_STATUS_SUCCESS) { return status; }
      }
      return HSA_STATUS_SUCCESS;
    }

    amd::elf::Segment* AmdHsaCode::MachineCode()
    {
      /*
      if (!segments[PT_AMDGPU_HSA_LOAD_CODE_AGENT - PT_LOOS][0]) {
        segments[PT_AMDGPU_HSA_LOAD_CODE_AGENT - PT_LOOS][0] = img->addSegment(PT_AMDGPU_HSA_LOAD_CODE_AGENT, 0, PF_R | PF_W | PF_X, 0);
      }
      return segments[PT_AMDGPU_HSA_LOAD_CODE_AGENT - PT_LOOS][0];
      */
      return 0;
    }

    amd::elf::Section* AmdHsaCode::HsaText()
    {
      if (!hsatext) {
        hsatext = MachineCode()->addSection(".hsatext",
          SHT_PROGBITS, (uint16_t) (SHF_ALLOC | SHF_EXECINSTR | SHF_WRITE | SHF_AMDGPU_HSA_CODE | SHF_AMDGPU_HSA_AGENT),
          0, 0, 0);
        img->symtab()->addSymbol(hsatext, "__hsa_codehsatext", ELF64_ST_INFO(STB_LOCAL, STT_SECTION), 0, 0, 0);
      }
      return hsatext;
    }

    void AmdHsaCode::AddHsaTextData(const void* buffer, size_t size)
    {
//      HsaText()->addData(buffer, size);
    }

    void AmdHsaCode::AddData(amdgpu_hsa_elf_section_t s, const void* data, size_t size)
    {
//      getDataSection(s)->addData(data, size);
    }

    void AmdHsaCode::AddImageInit(
      amdgpu_hsa_metadata_kind16_t kind,
      amdgpu_hsa_image_geometry8_t geometry,
      amdgpu_hsa_image_channel_order8_t channel_order, amdgpu_hsa_image_channel_type8_t channel_type,
      uint64_t width, uint64_t height, uint64_t depth, uint64_t array)
    {
      /*
      amdgpu_hsa_image_descriptor_t desc;
      desc.kind = kind;
      desc.geometry = geometry;
      desc.channel_order = channel_order;
      desc.channel_type = channel_type;
      desc.width = width;
      desc.height = height;
      desc.depth = depth;
      desc.array = array;
      uint64_t offset = ImageInit()->addData(&desc, sizeof(desc));
      amd::elf::Symbol* s = 
      img->symtab()->addSymbol(ImageInit(), "", ELF64_ST_INFO(STB_LOCAL, STT_AMDGPU_HSA_METADATA), 0, offset, 0);
      ImageInit()->reloc()->addRelocation(offset, s, R_AMDGPU_INIT_IMAGE);
      */
    }

    bool AmdHsaCode::PrintToFile(const std::string& filename)
    {
      std::ofstream out(filename);
      if (out.fail()) { return false; }
      Print(out);
      return out.fail();
    }

    void AmdHsaCode::Print(std::ostream& out)
    {
      PrintNotes(out);
      out << std::endl;
      PrintSegments(out);
      out << std::endl;
      PrintSections(out);
      out << std::endl;
      PrintSymbols(out);
      out << std::endl;
      PrintMachineCode(out);
      out << "AMD HSA Code Object End" << std::endl;
    }

    void AmdHsaCode::PrintNotes(std::ostream& out)
    {
      {
        uint32_t major_version, minor_version;
        if (GetNoteCodeObjectVersion(&major_version, &minor_version)) {
          out << "AMD HSA Code Object" << std::endl
              << "  Version " << major_version << ":" << minor_version << std::endl;
        }
      }
      {
        uint32_t hsail_major, hsail_minor;
        hsa_profile_t profile;
        hsa_machine_model_t machine_model;
        hsa_default_float_rounding_mode_t rounding_mode;
        if (GetNoteHsail(&hsail_major, &hsail_minor, &profile, &machine_model, &rounding_mode)) {
          out << "HSAIL " << std::endl
              << "  Version " << hsail_major << ":" << hsail_minor << std::endl
              << "  Profile " << (profile)
              << "  Machine model: " << (machine_model)
              << "  Default float rounding: " << (rounding_mode) << std::endl;
        }
      }
      {
        std::string vendor_name, architecture_name;
        uint32_t major_version, minor_version, stepping;
        if (GetNoteIsa(vendor_name, architecture_name, &major_version, &minor_version, &stepping)) {
          out << "ISA" << std::endl
              << "  Vendor " << vendor_name
              << "  Arch " << architecture_name
              << "  Version " << major_version << ":" << minor_version << ":" << stepping << std::endl;
        }
      }
      {
        std::string producer_name, producer_options;
        uint32_t major, minor;
        if (GetNoteProducer(&major, &minor, producer_name)) {
          out << "Producer '" << producer_name << "' " << "Version " << major << ":" << minor << std::endl;
        }
      }
      {
        std::string producer_options;
        if (GetNoteProducerOptions(producer_options)) {
          out << "Producer options" << std::endl
              << "  '" << producer_options << "'" << std::endl;
        }
      }
    }

    void AmdHsaCode::PrintSegments(std::ostream& out)
    {
      out << "Segments (total " << DataSegmentCount() << "):" << std::endl;
      for (size_t i = 0; i < DataSegmentCount(); ++i) {
        PrintSegment(out, DataSegment(i));
      }
    }

    void AmdHsaCode::PrintSections(std::ostream& out)
    {
      out << "Data Sections (total " << DataSectionCount() << "):" << std::endl;
      for (size_t i = 0; i < DataSectionCount(); ++i) {
        PrintSection(out, DataSection(i));
      }
      out << std::endl;
      out << "Relocation Sections (total " << RelocationSectionCount() << "):" << std::endl;
      for (size_t i = 0; i < RelocationSectionCount(); ++i) {
        PrintSection(out, GetRelocationSection(i));
      }
    }

    void AmdHsaCode::PrintSymbols(std::ostream& out)
    {
      out << "Symbols (total " << SymbolCount() << "):" << std::endl;
      for (size_t i = 0; i < SymbolCount(); ++i) {
        PrintSymbol(out, GetSymbol(i));
      }
    }

    void AmdHsaCode::PrintMachineCode(std::ostream& out)
    {
      for (size_t i = 0; i < SymbolCount(); ++i) {
        if (GetSymbol(i)->IsKernelSymbol() && GetSymbol(i)->IsDefinition()) {
          PrintMachineCode(out, GetSymbol(i)->AsKernelSymbol());
        }
      }
    }

    void AmdHsaCode::PrintSegment(std::ostream& out, Segment* segment)
    {
      out << "  Segment (" << segment->getSegmentIndex() << ")" << std::endl;
      out << "    Type: " << segment->type()
          << " "
          << "    Flags: " << "0x" << std::hex << std::setw(8) << std::setfill('0') << segment->flags() << std::dec
          << std::endl
          << "    Image Size: " << segment->imageSize()
          << " "
          << "    Memory Size: " << segment->memSize()
          << " "
          << "    Align: " << segment->align()
          << " "
          << "    VAddr: " << segment->vaddr()
          << std::endl;
      out << std::dec;
    }

    void AmdHsaCode::PrintSection(std::ostream& out, Section* section)
    {
      out << "  Section " << section->Name() << " (Index " << section->getSectionIndex() << ")" << std::endl;
      out << "    Type: " << section->type()
          << " "
          << "    Flags: " << "0x" << std::hex << std::setw(8) << std::setfill('0') << section->flags() << std::dec
          << std::endl
          << "    Size:  " << section->size()
          << " "
          << "    Address: " << section->addr()
          << " "
          << "    Align: " << section->addralign()
          << std::endl;
      out << std::dec;

      if (section->flags() & SHF_AMDGPU_HSA_CODE) {
        // Printed separately.
        return;
      }

      switch (section->type()) {
      case SHT_NOBITS:
        return;
      case SHT_RELA:
        PrintRelocationData(out, section->asRelocationSection());
        return;
      default:
        PrintRawData(out, section);
      }
    }

    void AmdHsaCode::PrintRawData(std::ostream& out, Section* section)
    {
      out << "    Data:" << std::endl;
      unsigned char *sdata = (unsigned char*)alloca(section->size());
      section->getData(0, sdata, section->size());
      PrintRawData(out, sdata, section->size());
    }

    void AmdHsaCode::PrintRawData(std::ostream& out, const unsigned char *data, size_t size)
    {
      out << std::hex << std::setfill('0');
      for (size_t i = 0; i < size; i += 16) {
        out << "      " << std::setw(7) << i << ":";

        for (size_t j = 0; j < 16; j += 1) {
          uint32_t value = i + j < size ? (uint32_t)data[i + j] : 0;
          if (j % 2 == 0) { out << ' '; }
          out << std::setw(2) << value;
        }
        out << "  ";

        for (size_t j = 0; i + j < size && j < 16; j += 1) {
          char value = (char)data[i + j] >= 32 && (char)data[i + j] <= 126 ? (char)data[i + j] : '.';
          out << value;
        }
        out << std::endl;
      }
      out << std::dec;
    }

    void AmdHsaCode::PrintRelocationData(std::ostream& out, RelocationSection* section)
    {
      out << "    Relocation Entries for " << section->targetSection()->Name() << " Section (total " << section->relocationCount() << "):" << std::endl;
      for (size_t i = 0; i < section->relocationCount(); ++i) {
        out << "      Relocation (Index " << i << "):" << std::endl;
        out << "        Type: " << section->relocation(i)->type() << std::endl;
        out << "        Symbol: " << section->relocation(i)->symbol()->name() << std::endl;
        out << "        Offset: " << section->relocation(i)->offset() << " Addend: " << section->relocation(i)->addend() << std::endl;
      }
      out << std::dec;
    }

    void AmdHsaCode::PrintSymbol(std::ostream& out, Symbol* sym)
    {
      out << "  Symbol " << sym->Name() << " (Index " << sym->Index() << "):" << std::endl;
      out << "    Section: " << sym->GetSection()->Name() << " ";
      out << "    Section Offset: " << sym->SectionOffset() << std::endl;
      out << "    VAddr: " << sym->VAddr() << " ";
      out << "    Size: " << sym->Size() << " ";
      out << "    Alignment: " << sym->Alignment() << std::endl;
      out << "    Kind: " << HsaSymbolKindToString(sym->Kind()) << " ";
      out << "    Linkage: " << HsaSymbolLinkageToString(sym->Linkage()) << " ";
      out << "    Definition: " << (sym->IsDefinition() ? "TRUE" : "FALSE") << std::endl;
      if (sym->IsVariableSymbol()) {
        out << "    Allocation: " << HsaVariableAllocationToString(sym->Allocation()) << " ";
        out << "    Segment: " << HsaVariableSegmentToString(sym->Segment()) << " ";
        out << "    Constant: " << (sym->IsConst() ? "TRUE" : "FALSE") << std::endl;
      }
      out << std::dec;
    }

    void AmdHsaCode::PrintMachineCode(std::ostream& out, KernelSymbol* sym)
    {
      amd_kernel_code_t kernel_code;
      sym->GetSection()->getData(sym->SectionOffset(), &kernel_code, sizeof(amd_kernel_code_t));

      out << "AMD Kernel Code for " << sym->Name() << ": " << std::endl << std::dec;
      PrintAmdKernelCode(out, &kernel_code);
      out << std::endl;

      if (sym->Size() > 0) {
        // \todo kzhuravl 8/18/2015 exclude sizeof(amd_kernel_code_t) from
        // sym->Size().
        uint64_t isa_size = sym->Size() - sizeof(amd_kernel_code_t);
        uint64_t isa_offset = sym->SectionOffset() + kernel_code.kernel_code_entry_byte_offset;
        unsigned char *isa = (unsigned char*)alloca(isa_size);
        sym->GetSection()->getData(isa_offset, isa, isa_size);

        out << "Disassembly for " << sym->Name() << ": " << std::endl;
        PrintDisassembly(out, isa, isa_size);
      }
      out << std::dec;
    }

    void AmdHsaCode::PrintDisassembly(std::ostream& out, const unsigned char *isa, size_t size)
    {
      PrintRawData(out, isa, size);
      out << std::dec;
    }

    std::string AmdHsaCode::MangleSymbolName(const std::string& module_name, const std::string symbol_name)
    {
      if (module_name.empty()) {
        return symbol_name;
      } else {
        return module_name + "::" + symbol_name;
      }
    }

      AmdHsaCode* AmdHsaCodeManager::FromHandle(hsa_code_object_t c)
      {
        CodeMap::iterator i = codeMap.find(c.handle);
        if (i == codeMap.end()) {
          AmdHsaCode* code = new AmdHsaCode();
          const void* buffer = reinterpret_cast<const void*>(c.handle);
          if (!code->InitAsBuffer(buffer, 0)) {
            delete code;
            return 0;
          }
          codeMap[c.handle] = code;
          return code;
        }
        return i->second;
      }

      bool AmdHsaCodeManager::Destroy(hsa_code_object_t c)
      {
        CodeMap::iterator i = codeMap.find(c.handle);
        if (i == codeMap.end()) {
          // Currently, we do not always create map entry for every code object buffer.
          return true;
        }
        codeMap.erase(i);
        return true;
      }
}
}
}
