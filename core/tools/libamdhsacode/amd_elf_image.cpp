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

#include "amd_elf_image.hpp"
#include <gelf.h>
#include <errno.h>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include "util.hpp"
#include "amd_hsa_code_util.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32
#include <libelf.h>

#ifndef _WIN32
#define _open open
#define _close close
#define _read read
#define _write write
#define _lseek lseek
#define _ftruncate ftruncate
#define _tempnam tempnam
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#else
#define _ftruncate _chsize
#endif

#if !defined(BSD_LIBELF)
  #define elf_setshstrndx elfx_update_shstrndx
#endif

using namespace ext;

namespace amd {
  namespace elf {

    class FileImage {
    public:
      FileImage();
      ~FileImage();
      bool create();
      bool readFrom(const std::string& filename);
      bool copyFrom(const void* data, size_t size);
      bool writeTo(const std::string& filename);

      std::string output() { return out.str(); }

      int fd() { return d; }

    private:
      int d;
      std::ostringstream out;

      bool error(const char* msg);
      bool perror(const char *msg);
      std::string werror();
    };

    FileImage::FileImage()
      : d(-1)
    {
    }

    FileImage::~FileImage()
    {
      if (d >= 0) { amd::hsa::CloseTempFile(d); }
    }

    bool FileImage::error(const char* msg)
    {
      out << "Error: " << msg << std::endl;
      return false;
    }

    bool FileImage::perror(const char* msg)
    {
      out << "Error: " << msg << ": " << strerror(errno) << std::endl;
      return false;
    }

#ifdef _WIN32
    std::string FileImage::werror()
    {
      LPVOID lpMsgBuf;
      DWORD dw = GetLastError();

      FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);
      std::string result((LPTSTR)lpMsgBuf);
      LocalFree(lpMsgBuf);
      return result;
    }
#endif // _WIN32

    bool FileImage::create()
    {
      d = amd::hsa::OpenTempFile("amdelf");
      if (d < 0) { return error("Failed to open temporary file for elf image"); }
      return true;
    }

    bool FileImage::readFrom(const std::string& filename)
    {
#ifdef _WIN32
      std::unique_ptr<char> buffer(new char[32 * 1024 * 1024]);
      HANDLE in = CreateFile(filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (in == INVALID_HANDLE_VALUE) { out << "Failed to open " << filename << ": " << werror() << std::endl; return false; }
      DWORD read;
      unsigned write;
      int written;
      do {
        if (!ReadFile(in, buffer.get(), sizeof(buffer), &read, NULL)) {
          out << "Failed to read " << filename << ": " << werror() << std::endl;
          CloseHandle(in);
          return false;
        }
        if (read > 0) {
          write = read;
          do {
            written = _write(d, buffer.get(), write);
            if (written < 0) {
              out << "Failed to write image file: " << werror() << std::endl;
              CloseHandle(in);
            }
            write -= written;
          } while (write > 0);
        }
      } while (read > 0);
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek(0) failed"); }
      CloseHandle(in);
      return true;
#else // _WIN32
      int in = _open(filename.c_str(), O_RDONLY);
      if (in < 0) { return perror("open failed"); }
      if (_lseek(in, 0L, SEEK_END) < 0) { return perror("lseek failed"); }
      off_t size;
      if ((size = _lseek(in, 0L, SEEK_CUR)) < 0) { return perror("lseek(2) failed"); }
      if (_lseek(in, 0L, SEEK_SET) < 0) { return perror("lseek(3) failed"); }
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek(3) failed"); }
      ssize_t written;
      do {
        written = sendfile(d, in, NULL, size);
        if (written < 0) {
          _close(in);
          return perror("sendfile failed");
        }
        size -= written;
      } while (size > 0);
      _close(in);
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek(0) failed"); }
      return true;
#endif // _WIN32
    }

    bool FileImage::copyFrom(const void* data, size_t size)
    {
      assert(d != -1);
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek failed"); }
      if (_ftruncate(d, 0) < 0) { return perror("ftruncate failed"); }
      int written, offset = 0;
      while (size > 0) {
        written = _write(d, (const char*) data + offset, size);
        if (written < 0) {
          return perror("write failed");
        }
        size -= written;
        offset += written;
      }
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek failed"); }
      return true;
    }

    bool FileImage::writeTo(const std::string& filename)
    {
      assert(d != -1);
      if (_lseek(d, 0L, SEEK_END) < 0) { return perror("lseek failed"); }
      long size;
      if ((size = _lseek(d, 0L, SEEK_CUR)) < 0) { return perror("lseek(2) failed"); }
      if (_lseek(d, 0L, SEEK_SET) < 0) { return perror("lseek(3) failed"); }
      char* buffer = (char*)malloc(size);
      if (_read(d, buffer, size) < 0) { return perror("read failed"); }
      std::ofstream out(filename.c_str(), std::ios::binary);
      out.write(buffer, size);
      free(buffer);
      return true;
    }

    class Buffer {
    public:
      Buffer();
      Buffer(const char* ptr, size_t size);
      void init(const char* ptr, size_t size);

      const char* ptr() { return dataptr; }
      size_t size() { return isConst() ? datasize : data.size(); }

      bool isConst() const { return datasize != 0; }
      bool has(const char* ptr);
      uint16_t getStringNdx(const char* ptr);
      const char* addString0(const std::string& s);
      uint16_t addString1(const std::string& s);
      const char* getString0(uint16_t offset);
      void addStringLength(const std::string& name) { addU32(name.length() + 1); }
      void addU32(uint32_t u);
      void addData(const void* desc, uint32_t desc_size) { data.insert(data.end(), (const char*) desc, (const char*) desc + desc_size); dataptr = data.data(); }

    private:
      std::vector<char> data;
      const char* dataptr;
      size_t datasize;
    };

    Buffer::Buffer()
      : dataptr(data.data()),
        datasize(0)
    {
    }

    Buffer::Buffer(const char* ptr_, size_t size_)
      : dataptr(ptr_),
      datasize(size_)
    {
    }

    void Buffer::init(const char* ptr, size_t datasize)
    {
      assert(size() == 0);
      this->dataptr = ptr;
      this->datasize = datasize;
    }

    const char* Buffer::addString0(const std::string& s)
    {
      uint16_t index = addString1(s);
      return dataptr + index;
    }

    uint16_t Buffer::addString1(const std::string& s)
    {
      assert(!isConst());
      size_t osize = size();
      data.insert(data.end(), s.begin(), s.end());
      data.push_back('\0');
      dataptr = data.data();
      return osize;
    }

    const char* Buffer::getString0(uint16_t offset)
    {
      return dataptr + offset;
    }

    void Buffer::addU32(uint32_t u)
    {
      data.push_back((u >> 24) & 0xFF);
      data.push_back((u >> 16) & 0xFF);
      data.push_back((u >> 8) & 0xFF);
      data.push_back(u & 0xFF);
    }
    bool Buffer::has(const char* ptr)
    {
      return (ptr >= dataptr) && (ptr < dataptr + size());
    }

    uint16_t Buffer::getStringNdx(const char* ptr)
    {
      assert(has(ptr));
      return ptr - dataptr;
    }

    class GElfImage;
    class GElfSegment;

    class GElfSection : public virtual Section {
    public:
      GElfSection(GElfImage* elf, GElfSegment* segment_ = 0);

      bool push(const char* name, uint32_t shtype, uint64_t shflags, uint16_t shlink, uint32_t info, uint32_t align);
      bool pull(uint16_t ndx);
      virtual bool pullData() { return true; }
      bool updateSegment(GElf_Phdr* phdr);
      bool push();
      uint16_t getSectionIndex() const override;
      uint32_t type() const override { return hdr.sh_type; }
      std::string Name() const override { return name; }
      uint64_t addr() const override { return hdr.sh_addr; }
      uint64_t addralign() const override { return hdr.sh_addralign; }
      uint64_t flags() const override { return hdr.sh_flags; }
      uint64_t size() const override { return hdr.sh_size; }
      bool getData(uint64_t offset, void* dest, uint64_t size) override;
      uint64_t addData(const void* data, size_t size);
      Segment* segment() override { return seg; }
      RelocationSection* asRelocationSection() override { return 0; }

    protected:
      const char* name;
      GElfImage* elf;
      Segment* seg;
      Elf_Scn* scn;
      GElf_Shdr hdr;
      Elf_Data* edata0, *edata;
      Buffer data0, data;
      Buffer& newdata();
      friend class GElfSymbol;
    };

    class GElfSegment : public Segment {
    public:
      GElfSegment(GElfImage* elf, uint16_t index);
      bool push(uint32_t type, uint16_t vaddr, uint32_t flags, uint64_t align);
      bool push();
      bool pull();
      uint64_t type() const override { return phdr.p_type; }
      uint64_t memSize() const override { return phdr.p_memsz; }
      uint64_t align() const override { return phdr.p_align; }
      uint64_t imageSize() const override { return phdr.p_filesz; }
      uint64_t vaddr() const override { return phdr.p_vaddr; }
      uint64_t flags() const override { return phdr.p_flags; }
      const char* data() const override;
      uint16_t getSegmentIndex() override;
      Section* addSection(const std::string& name, uint32_t type, uint16_t flags, uint32_t link, uint32_t info, uint64_t align);

    private:
      GElfImage* elf;
      uint16_t index;
      GElf_Phdr phdr;
      std::vector<std::unique_ptr<GElfSection>> sections;
    };

    class GElfStringTable : public GElfSection, public StringTable {
    public:
      GElfStringTable(GElfImage* elf);
      bool push(const char* name, uint32_t shtype, uint64_t shflags);
      bool pullData() override;
      const char* addString(const std::string& s) override;
      uint16_t addString1(const std::string& s);
      const char* getString(uint16_t ndx) override;
      uint16_t getStringIndex(const char* name) override;

      uint16_t getSectionIndex() const override { return GElfSection::getSectionIndex(); }
      uint32_t type() const override { return GElfSection::type(); }
      std::string Name() const override { return GElfSection::Name(); }
      uint64_t addr() const override { return GElfSection::addr(); }
      uint64_t addralign() const override { return GElfSection::addralign(); }
      uint64_t flags() const override { return GElfSection::flags(); }
      uint64_t size() const override { return GElfSection::size(); }
      Segment* segment() override { return GElfSection::segment(); }
      bool getData(uint64_t offset, void* dest, uint64_t size) override { return GElfSection::getData(offset, dest, size); }
      RelocationSection* asRelocationSection() override { return 0; }
    };

    class GElfSymbolTable;

    class GElfSymbol : public Symbol {
    public:
      GElfSymbol(GElfSymbolTable* symtab, Elf_Data* edata, int index);

      bool push(const std::string& name, uint64_t value, uint64_t size, unsigned char type, unsigned char binding, uint16_t shndx, unsigned char other);
      bool pull();

      uint32_t index() override { return eindex; }
      uint32_t type() override { return ELF64_ST_TYPE(sym.st_info); }
      uint32_t binding() { return ELF64_ST_BIND(sym.st_info); }
      uint64_t size() { return sym.st_size; }
      uint64_t value() { return sym.st_value; }
      unsigned char other() { return sym.st_other; }
      std::string name() const override;
      Section* section();

    private:
      GElfSymbolTable* symtab;
      Elf_Data* edata;
      int eindex;
      GElf_Sym sym;
      friend class GElfSymbolTable;
    };

    class GElfSymbolTable : public GElfSection, public SymbolTable {
    private:
      GElfStringTable* strtab;
      std::vector<std::unique_ptr<GElfSymbol>> symbols;
      friend class GElfSymbol;

    public:
      GElfSymbolTable(GElfImage* elf);
      bool push(const char* name, GElfStringTable* strtab);
      bool pullData() override;
      uint16_t getSectionIndex() const override { return GElfSection::getSectionIndex(); }
      uint32_t type() const override { return GElfSection::type(); }
      std::string Name() const override { return GElfSection::Name(); }
      uint64_t addr() const override { return GElfSection::addr(); }
      uint64_t addralign() const override { return GElfSection::addralign(); }
      uint64_t flags() const override { return GElfSection::flags(); }
      uint64_t size() const override { return GElfSection::size(); }
      Segment* segment() override { return GElfSection::segment(); }
      bool getData(uint64_t offset, void* dest, uint64_t size) override { return GElfSection::getData(offset, dest, size); }
      Symbol* addSymbol(Section* section, const std::string& name, uint64_t value, uint64_t size, unsigned char type, unsigned char binding, unsigned char other = 0) override;
      size_t symbolCount() override;
      Symbol* symbol(size_t i) override;
      RelocationSection* asRelocationSection() override { return 0; }
    };

    class GElfNoteSection : public GElfSection, public NoteSection {
    public:
      GElfNoteSection(GElfImage* elf);
      bool push(const std::string& name);
      uint16_t getSectionIndex() const override { return GElfSection::getSectionIndex(); }
      uint32_t type() const override { return GElfSection::type(); }
      std::string Name() const override { return GElfSection::Name(); }
      uint64_t addr() const override { return GElfSection::addr(); }
      uint64_t addralign() const override { return GElfSection::addralign(); }
      uint64_t flags() const override { return GElfSection::flags(); }
      uint64_t size() const override { return GElfSection::size(); }
      Segment* segment() override { return GElfSection::segment(); }
      bool getData(uint64_t offset, void* dest, uint64_t size) override { return GElfSection::getData(offset, dest, size); }
      bool addNote(const std::string& name, uint32_t type, const void* desc, uint32_t desc_size) override;
      bool getNote(const std::string& name, uint32_t type, void** desc, uint32_t* desc_size) override;
      RelocationSection* asRelocationSection() override { return 0; }
    };

    class GElfRelocationSection;

    class GElfRelocation : public Relocation {
    private:
      GElfRelocationSection* rsection;
      Elf_Data* edata;
      int eindex;
      GElf_Rela rela;

    public:
      GElfRelocation(GElfRelocationSection* rsection_, Elf_Data* edata_, int eindex_)
        : rsection(rsection_),
          edata(edata_), eindex(eindex_)
      {
      }

      bool push(uint32_t type, Symbol* symbol, uint64_t offset, uint64_t addend);
      bool pull();

      RelocationSection* section() override;
      uint32_t type() const override { return GELF_R_TYPE(rela.r_info); }
      uint32_t symbolIndex() const override { return GELF_R_SYM(rela.r_info); }
      Symbol* symbol() const override;
      uint64_t offset() const override { return rela.r_offset; }
      uint64_t addend() const override { return rela.r_addend; }
    };

    class GElfRelocationSection : public GElfSection, public RelocationSection {
    private:
      Section* section;
      GElfSymbolTable* symtab;
      std::vector<std::unique_ptr<Relocation>> relocations;

    public:
      GElfRelocationSection(GElfImage* elf, Section* targetSection = 0, GElfSymbolTable* symtab_ = 0);
      bool push(const std::string& name);
      bool pullData() override;
      uint16_t getSectionIndex() const override { return GElfSection::getSectionIndex(); }
      uint32_t type() const override { return GElfSection::type(); }
      std::string Name() const override { return GElfSection::Name(); }
      uint64_t addr() const override { return GElfSection::addr(); }
      uint64_t addralign() const override { return GElfSection::addralign(); }
      uint64_t flags() const override { return GElfSection::flags(); }
      uint64_t size() const override { return GElfSection::size(); }
      Segment* segment() override { return GElfSection::segment(); }
      bool getData(uint64_t offset, void* dest, uint64_t size) override { return GElfSection::getData(offset, dest, size); }
      RelocationSection* asRelocationSection() override { return this; }

      size_t relocationCount() const override { return relocations.size(); }
      Relocation* relocation(size_t i) override { return relocations[i].get(); }
      Relocation* addRelocation(uint32_t type, Symbol* symbol, uint64_t offset, uint64_t addend) override;
      Section* targetSection() override { return section; }
      friend class GElfRelocation;
    };

    class GElfImage : public Image {
    public:
      GElfImage(int elfclass);
      ~GElfImage();
      bool initNew(uint16_t machine, uint16_t type);
      bool loadFromFile(const std::string& filename) override;
      bool saveToFile(const std::string& filename) override;
      bool initFromBuffer(const void* buffer, size_t size);
      bool initAsBuffer(const void* buffer, size_t size);
      bool close();
      bool writeTo(const std::string& filename) override;

      const char* data() override { assert(buffer); return buffer; }
      uint64_t size() override { assert(buffer); return ElfSize(buffer); }

      bool push();

      bool Validate() override;

      uint16_t Machine() override { return ehdr.e_machine; }
      uint16_t Type() override { return ehdr.e_type; }

      GElfStringTable* shstrtab();
      GElfStringTable* strtab();
      GElfSymbolTable* getSymtab(uint16_t index)
      {
        return static_cast<GElfSymbolTable*>(section(index));
      }

      GElfStringTable* addStringTable(const std::string& name) override;
      GElfStringTable* getStringTable(uint16_t index) override;

      GElfSymbolTable* addSymbolTable(const std::string& name, StringTable* stab = 0) override;
      GElfSymbolTable* symtab();

      size_t reserveSegments(size_t count) override;
      Segment* addSegment(uint32_t type, uint16_t vaddr, uint32_t flags, uint64_t align) override;
      size_t segmentCount() override { return segments.size(); }
      GElfSegment* segment(size_t i) override { return segments[i].get(); }
      Segment* segmentByVAddr(uint64_t vaddr) override;
      size_t sectionCount() override { return sections.size(); }
      GElfSection* section(size_t i) override { return sections[i].get(); }
      uint16_t machine() const;
      uint16_t etype() const;
      int eclass() const { return elfclass; }
      bool elfError(const char* msg);

      GElfNoteSection* note() override;
      GElfNoteSection* addNoteSection(const std::string& name) override;

    private:
      int elfclass;
      FileImage img;
      const char* buffer;
      size_t bufferSize;
      Elf* e;
      GElf_Ehdr ehdr;
      GElfStringTable* shstrtabSection;
      GElfStringTable* strtabSection;
      GElfSymbolTable* symtabSection;
      GElfNoteSection* noteSection;
      std::vector<std::unique_ptr<GElfSegment>> segments;
      std::vector<std::unique_ptr<GElfSection>> sections;

      bool imgError();
      const char *elfError();
      bool elfBegin(Elf_Cmd cmd);
      bool elfEnd();
      bool push0();
      bool pullElf();

      friend class GElfSection;
      friend class GElfSegment;
      friend class GElfSymbol;
    };

    GElfSegment::GElfSegment(GElfImage* elf_, uint16_t index_)
      : elf(elf_),
        index(index_)
    {
      memset(&phdr, 0, sizeof(phdr));
    }

    const char* GElfSegment::data() const
    {
      return (const char*) elf->data() + phdr.p_offset;
    }

    bool GElfImage::Validate()
    {
      if (ELFMAG0 != ehdr.e_ident[EI_MAG0] ||
          ELFMAG1 != ehdr.e_ident[EI_MAG1] ||
          ELFMAG2 != ehdr.e_ident[EI_MAG2] ||
          ELFMAG3 != ehdr.e_ident[EI_MAG3]) {
        out << "Invalid ELF magic" << std::endl;
        return false;
      }
      if (EV_CURRENT != ehdr.e_version) {
        out << "Invalid ELF version" << std::endl;
        return false;
      }
      return true;
    }

    bool GElfSegment::push(uint32_t type, uint16_t vaddr, uint32_t flags, uint64_t align)
    {
      phdr.p_type = type;
      phdr.p_vaddr = vaddr;
      phdr.p_flags = flags;
      phdr.p_align = align;
      phdr.p_memsz = 0;
      phdr.p_filesz = 1; // Update later when sections are added.
      if (!gelf_update_phdr(elf->e, index, &phdr)) { return elf->elfError("gelf_update_phdr failed"); }
      return true;
    }

    bool GElfSegment::push()
    {
      if (!gelf_getphdr(elf->e, index, &phdr)) { return elf->elfError("gelf_getphdr failed"); }
      phdr.p_filesz = 0;
      phdr.p_memsz = 0;
      phdr.p_align = 0;
      for (std::unique_ptr<GElfSection>& section : sections) {
        if (!section->updateSegment(&phdr)) { return false; }
      }
      if (!gelf_update_phdr(elf->e, index, &phdr)) { return elf->elfError("gelf_update_phdr failed"); }
      return true;
    }

    bool GElfSegment::pull()
    {
      if (!gelf_getphdr(elf->e, index, &phdr)) { return elf->elfError("gelf_getphdr failed"); }
      return true;
    }

    uint16_t GElfSegment::getSegmentIndex()
    {
      return index;
    }

    Section* GElfSegment::addSection(const std::string& name, uint32_t type, uint16_t flags, uint32_t link, uint32_t info, uint64_t align)
    {
      GElfSection* section = new GElfSection(elf, this);
      section->push(name.c_str(), type, flags, link, info, align);
      sections.push_back(std::unique_ptr<GElfSection>(section));
      return section;
    }

    GElfSection::GElfSection(GElfImage* elf_, GElfSegment* segment_)
      : name(0),
        elf(elf_),
        seg(segment_),
        scn(0)
    {
    }

    uint16_t GElfSection::getSectionIndex() const
    {
      return elf_ndxscn(scn);
    }

    bool GElfSection::push(const char* name, uint32_t shtype, uint64_t shflags, uint16_t shlink, uint32_t info, uint32_t align)
    {
      scn = elf_newscn(elf->e);
      if (!scn) { return false; }
      if (!gelf_getshdr(scn, &hdr)) { return elf->elfError("gelf_get_shdr failed"); }
      this->name = elf->shstrtab()->addString(name);
      hdr.sh_name = elf->shstrtab()->getStringIndex(this->name);
      hdr.sh_type = shtype;
      hdr.sh_flags = shflags;
      hdr.sh_link = shlink;
      hdr.sh_info = info;
      hdr.sh_addralign = align;
      if (!gelf_update_shdr(scn, &hdr)) { return elf->elfError("gelf_update_shdr failed"); }
      edata0 = 0;
      return true;
    }

    bool GElfSection::pull(uint16_t ndx)
    {
      scn = elf_getscn(elf->e, ndx);
      if (!scn) { return false; }
      if (!gelf_getshdr(scn, &hdr)) { return elf->elfError("gelf_get_shdr failed"); }
      edata0 = elf_getdata(scn, NULL);
      if (edata0) { data0.init((const char*)edata0->d_buf, edata0->d_size); }
      this->name = elf->shstrtab()->getString(hdr.sh_name);
      seg = elf->segmentByVAddr(hdr.sh_addr);
      return true;
    }

    bool GElfSection::updateSegment(GElf_Phdr* phdr)
    {
      phdr->p_filesz += hdr.sh_size;
      phdr->p_memsz = hdr.sh_type == SHT_NOBITS ? 0 : hdr.sh_size;
      phdr->p_align = (std::max)(phdr->p_align, hdr.sh_addralign);
      return true;
    }

    bool GElfSection::push()
    {
      if (edata && edata->d_size != data.size()) {
        edata->d_buf = (void*) data.ptr();
        edata->d_size = data.size();
      }
      return true;
    }

    Buffer& GElfSection::newdata()
    {
      if (data.size() == 0) {
        edata = elf_newdata(scn);
        edata->d_buf = (void*) this->data.ptr();
      }
      return data;
    }

    uint64_t GElfSection::addData(const void* data, size_t size)
    {
      assert(false);
      return 0;
    }

    bool GElfSection::getData(uint64_t offset, void* dest, uint64_t size)
    {
      Elf_Data* edata = 0;
      uint64_t coffset = 0;
      uint64_t csize = 0;
      if ((edata = elf_getdata(scn, edata)) != 0) {
        if (coffset <= offset && offset <= coffset + edata->d_size) {
          csize = (std::min)(size, edata->d_size - offset);
          memcpy(dest, (const char*) edata->d_buf + offset - coffset, csize);
          coffset += csize;
          dest = (char*) dest + csize;
          size -= csize;
          if (!size) { return true; }
        }
      }
      return false;
    }

    GElfStringTable::GElfStringTable(GElfImage* elf)
      : GElfSection(elf)
    {
    }

    bool GElfStringTable::push(const char* name, uint32_t shtype, uint64_t shflags)
    {
      if (!GElfSection::push(name, shtype, shflags, SHN_UNDEF, 0, 0)) { return false; }
      return true;
    }

    bool GElfStringTable::pullData()
    {
      return true;
    }

    const char* GElfStringTable::addString(const std::string& s)
    {
      newdata();
      if (data0.size() == 0 && data.size() == 0) { data.addString0(""); }
      return data.addString0(s);
    }

    uint16_t GElfStringTable::addString1(const std::string& s)
    {
      newdata();
      if (data0.size() == 0 && data.size() == 0) { data.addString0(""); }
      return data.addString1(s);
    }

    const char* GElfStringTable::getString(uint16_t ndx)
    {
      return data0.getString0(ndx);
    }

    uint16_t GElfStringTable::getStringIndex(const char* s)
    {
      if (data0.has(s)) { return data0.getStringNdx(s); }
      else if (data.has(s)) { return data.getStringNdx(s); }
      else { assert(false); return 0; }
    }

    GElfSymbol::GElfSymbol(GElfSymbolTable* symtab_, Elf_Data* edata_, int eindex_)
      : symtab(symtab_),
        edata(edata_),
        eindex(eindex_)
    {
    }

    Section* GElfSymbol::section()
    {
      if (sym.st_shndx != SHN_UNDEF) {
        return symtab->elf->section(sym.st_shndx);
      }
      return 0;
    }

    bool GElfSymbol::push(const std::string& name, uint64_t value, uint64_t size, unsigned char type, unsigned char binding, uint16_t shndx, unsigned char other)
    {
      sym.st_name = symtab->strtab->addString1(name.c_str());
      sym.st_value = value;
      sym.st_size = size;
      sym.st_info = GELF_ST_INFO(binding, type);
      sym.st_shndx = shndx;
      sym.st_other = other;
      if (!gelf_update_sym(edata, eindex, &sym)) { return symtab->elf->elfError("gelf_update_sym failed"); }
      return true;
    }

    bool GElfSymbol::pull()
    {
      if (!gelf_getsym(edata, eindex, &sym)) { return symtab->elf->elfError("gelf_getsym failed"); }      
      return true;
    }

    std::string GElfSymbol::name() const
    {
      return symtab->strtab->getString(sym.st_name);
    }

    GElfSymbolTable::GElfSymbolTable(GElfImage* elf)
      : GElfSection(elf),
        strtab(0)
    {
    }

    bool GElfSymbolTable::push(const char* name, GElfStringTable* strtab)
    {
      if (!strtab) { strtab = elf->strtab(); }
      this->strtab = strtab;
      if (!GElfSection::push(name, SHT_SYMTAB, SHF_ALLOC, strtab->getSectionIndex(), 0, 4)) { return false;  }
      return true;
    }

    bool GElfSymbolTable::pullData()
    {
      strtab = elf->getStringTable(hdr.sh_link);
      Elf_Data* d = 0;
      while ((d = elf_getdata(scn, d)) != 0) {
        if (d->d_size % sizeof(GElf_Sym) != 0) { return false; }
        for (size_t i = 0; i < d->d_size / sizeof(GElf_Sym); ++i) {
          GElfSymbol* sym = new GElfSymbol(this, d, i);
          if (!sym->pull()) { delete sym; return false; }
          symbols.push_back(std::unique_ptr<GElfSymbol>(sym));
        }
      }
      return true;
    }

    Symbol* GElfSymbolTable::addSymbol(Section* section, const std::string& name, uint64_t value, uint64_t size, unsigned char type, unsigned char binding, unsigned char other)
    {
      Elf_Data* data = elf_newdata(scn);
      data->d_align = 4;
      data->d_type = ELF_T_SYM;
      data->d_version = EV_CURRENT;
      data->d_buf = 0;
      edata->d_size = elf->eclass() == ELFCLASS32 ? sizeof(Elf32_Sym) : sizeof(Elf64_Sym);
      edata->d_buf = data;
      GElfSymbol* sym = new GElfSymbol(this, data, 0);
      uint16_t shndx = section ? section->getSectionIndex() : (uint16_t) SHN_UNDEF;
      if (!sym->push(name, value, size, type, binding, shndx, other)) { delete sym; return 0; }
      symbols.push_back(std::unique_ptr<GElfSymbol>(sym));
      return sym;
    }

    size_t GElfSymbolTable::symbolCount()
    {
      return symbols.size();
    }

    Symbol* GElfSymbolTable::symbol(size_t i)
    {
      return symbols[i].get();
    }

    GElfNoteSection::GElfNoteSection(GElfImage* elf)
      : GElfSection(elf)
    {
    }

    bool GElfNoteSection::push(const std::string& name)
    {
      return GElfSection::push(name.c_str(), SHT_NOTE, 0, 0, 0, 8);
    }

    bool GElfNoteSection::addNote(const std::string& name, uint32_t type, const void* desc, uint32_t desc_size)
    {
      newdata();
      data.addStringLength(name);
      data.addU32(desc_size);
      data.addU32(type);
      data.addString0(name);
      if (desc_size > 0) {
        assert(desc);
        data.addData(desc, desc_size);
      }
      return true;
    }

    bool GElfNoteSection::getNote(const std::string& name, uint32_t type, void** desc, uint32_t* desc_size)
    {
      Elf_Data* data = 0;
      while ((data = elf_getdata(scn, data)) != 0) {
        uint32_t note_offset = 0;
        while (note_offset < data->d_size) {
          char* notec = (char *) data->d_buf + note_offset;
          Elf64_Nhdr* note = (Elf64_Nhdr*) notec;
          if (type == note->n_type) {
            std::string note_name = GetNoteString(note->n_namesz, notec + sizeof(Elf64_Nhdr));
            if (name == note_name) {
              *desc = notec + sizeof(Elf64_Nhdr) + common::alignUp(note->n_namesz, 4);
              *desc_size = note->n_descsz;
              return true;
            }
          }
          note_offset += sizeof(Elf64_Nhdr) + common::alignUp(note->n_namesz, 4) + common::alignUp(note->n_descsz, 4);
        }
      }
      return false;
    }

    bool GElfRelocation::push(uint32_t type, Symbol* symbol, uint64_t offset, uint64_t addend)
    {
      rela.r_info = GELF_R_INFO((uint64_t) symbol->index(), type);
      rela.r_offset = offset;
      rela.r_addend = addend;
      if (!gelf_update_rela(edata, eindex, &rela)) { return rsection->elf->elfError("gelf_update_rela failed"); }
      return true;
    }

    bool GElfRelocation::pull()
    {
      if (!gelf_getrela(edata, eindex, &rela)) { return rsection->elf->elfError("gelf_getrela failed"); }
      return true;
    }

    RelocationSection* GElfRelocation::section()
    {
      return rsection;
    }

    Symbol* GElfRelocation::symbol() const
    {
      return rsection->symtab->symbol(symbolIndex());
    }

    GElfRelocationSection::GElfRelocationSection(GElfImage* elf, Section* section_, GElfSymbolTable* symtab_)
      : GElfSection(elf),
        section(section_),
        symtab(symtab_)
    {
    }

    Relocation* GElfRelocationSection::addRelocation(uint32_t type, Symbol* symbol, uint64_t offset, uint64_t addend)
    {
      Elf_Data* data = elf_newdata(scn);
      data->d_align = 4;
      data->d_type = ELF_T_RELA;
      data->d_version = EV_CURRENT;
      data->d_buf = 0;
      edata->d_size = elf->eclass() == ELFCLASS32 ? sizeof(Elf32_Rela) : sizeof(Elf64_Rela);
      edata->d_buf = data;
      GElfRelocation* rel = new GElfRelocation(this, data, 0);
      if (!rel->push(type, symbol, offset, addend)) { delete rel; return 0; }
      relocations.push_back(std::unique_ptr<Relocation>(rel));
      return rel;
    }

    bool GElfRelocationSection::pullData()
    {
      section = elf->section(hdr.sh_info);
      symtab = elf->getSymtab(hdr.sh_link);
      Elf_Data* d = 0;
      while ((d = elf_getdata(scn, d)) != 0) {
        if (d->d_size % sizeof(GElf_Rela) != 0) { return false; }
        for (size_t i = 0; i < d->d_size / sizeof(GElf_Rela); ++i) {
          GElfRelocation* rel = new GElfRelocation(this, d, i);
          if (!rel->pull()) { delete rel; return false; }
          relocations.push_back(std::unique_ptr<Relocation>(rel));
        }
      }
      return true;
    }

    GElfImage::GElfImage(int elfclass_)
      : elfclass(elfclass_),
        buffer(0), bufferSize(0),
        e(0),
        shstrtabSection(0), strtabSection(0),
        symtabSection(0),
        noteSection(0)
    {
      if (EV_NONE == elf_version(EV_CURRENT)) {
        assert(false);
      }
    }

    GElfImage::~GElfImage()
    {
      elf_end(e);
    }

    bool GElfImage::imgError()
    {
      out << img.output();
      return false;
    }

    const char *GElfImage::elfError()
    {
      return elf_errmsg(-1);
    }

    bool GElfImage::elfBegin(Elf_Cmd cmd)
    {
      if ((e = elf_begin(img.fd(), cmd, NULL
#ifdef AMD_LIBELF
                       , NULL
#endif
        )) == NULL) {
        out << "elf_begin failed: " << elfError() << std::endl;
        return false;
      }
      return true;
    }

    bool GElfImage::initNew(uint16_t machine, uint16_t type)
    {
      if (!img.create()) { return imgError(); }
      if (!elfBegin(ELF_C_WRITE)) { return false; }
      if (!gelf_newehdr(e, elfclass)) { return elfError("gelf_newehdr failed"); }
      if (!gelf_getehdr(e, &ehdr)) { return elfError("gelf_getehdr failed"); }
      ehdr.e_ident[EI_DATA] = ELFDATA2MSB;
      ehdr.e_machine = machine;
      ehdr.e_type = ET_DYN;
      ehdr.e_version = EV_CURRENT;
      if (!gelf_update_ehdr(e, &ehdr)) { return elfError("gelf_updateehdr failed"); }
      if (!shstrtab()->push(".shstrtab", SHT_STRTAB, SHF_STRINGS | SHF_ALLOC)) { return elfError("Failed to create shstrtab"); }
      //elf_setshstrndx(e, shstrtab()->getSectionIndex());
      ehdr.e_shstrndx = shstrtab()->getSectionIndex();
      if (!strtab()->push(".strtab", SHT_STRTAB, SHF_STRINGS | SHF_ALLOC)) { return elfError("Failed to create strtab"); }
      gelf_update_ehdr(e, &ehdr);
      return true;
    }

    bool GElfImage::loadFromFile(const std::string& filename)
    {
      if (!img.create()) { return imgError(); }
      if (!img.readFrom(filename)) { return imgError(); }
      if (!elfBegin(ELF_C_RDWR)) { return false; }
      return pullElf();
    }

    bool GElfImage::saveToFile(const std::string& filename)
    {
      if (buffer) {
        std::ofstream out(filename.c_str(), std::ios::binary);
        if (out.fail()) { return false; }
        out.write(buffer, bufferSize);
        return !out.fail();
      } else {
        return img.writeTo(filename);
      }
    }

    bool GElfImage::initFromBuffer(const void* buffer, size_t size)
    {
      if (size == 0) { size = ElfSize(buffer); }
      if (!img.create()) { return imgError(); }
      if (!img.copyFrom(buffer, size)) { return imgError(); }
      if (!elfBegin(ELF_C_RDWR)) { return false; }
      return pullElf();
    }

    bool GElfImage::initAsBuffer(const void* buffer, size_t size)
    {
      if (size == 0) { size = ElfSize(buffer); }
      if ((e = elf_memory(reinterpret_cast<char*>(const_cast<void*>(buffer)), size
#ifdef AMD_LIBELF
                       , NULL
#endif
        )) == NULL) {
        out << "elf_begin(buffer) failed: " << elfError() << std::endl;
        return false;
      }
      this->buffer = reinterpret_cast<const char*>(buffer);
      this->bufferSize = size;
      return pullElf();
    }

    bool GElfImage::pullElf()
    {
      if (!gelf_getehdr(e, &ehdr)) { return elfError("gelf_getehdr failed"); }
      segments.reserve(ehdr.e_phnum);
      for (size_t i = 0; i < ehdr.e_phnum; ++i) {
        GElfSegment* segment = new GElfSegment(this, i);
        segment->pull();
        segments.push_back(std::unique_ptr<GElfSegment>(segment));
      }

      shstrtabSection = new GElfStringTable(this);
      if (!shstrtabSection->pull(ehdr.e_shstrndx)) { return false; }

      Elf_Scn* scn = 0;
      size_t n = 0;
      sections.push_back(std::unique_ptr<GElfSection>());
      while ((scn = elf_nextscn(e, scn)) != 0) {
        if (n++ == ehdr.e_shstrndx) {
          sections.push_back(std::unique_ptr<GElfSection>(shstrtabSection));
          continue;
        }
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr)) { return elfError("Failed to get shdr"); }
        GElfSection* section = 0;
        if (shdr.sh_type == SHT_NOTE) {
          section = new GElfNoteSection(this);
        } else if (shdr.sh_type == SHT_RELA) {
          section = new GElfRelocationSection(this);
        } else if (shdr.sh_type == SHT_STRTAB) {
          section = new GElfStringTable(this);
        } else if (shdr.sh_type == SHT_SYMTAB) {
          section = new GElfSymbolTable(this);
        } else {
          section = new GElfSection(this);
        }
        if (section) {
          sections.push_back(std::unique_ptr<GElfSection>(section));
          if (!section->pull(n)) { return false; }
        }
      }

      for (size_t n = 1; n < sections.size(); ++n) {
        GElfSection* section = sections[n].get();
        if (section->type() == SHT_STRTAB) {
          if (!section->pullData()) { return false; }
        }
      }

      for (size_t n = 1; n < sections.size(); ++n) {
        GElfSection* section = sections[n].get();
        if (section->type() == SHT_SYMTAB) {
          if (!section->pullData()) { return false; }
        }
      }

      for (size_t n = 1; n < sections.size(); ++n) {
        GElfSection* section = sections[n].get();
        if (section->type() != SHT_STRTAB && section->type() != SHT_SYMTAB) {
          if (!section->pullData()) { return false; }
        }
      }

      for (size_t i = 1; i < sections.size(); ++i) {
        if (i == ehdr.e_shstrndx || i == ehdr.e_shstrndx) { continue; }
        std::unique_ptr<GElfSection>& section = sections[i];
        if (section->Name() == ".strtab") { strtabSection = static_cast<GElfStringTable*>(section.get()); }
        if (section->Name() == ".symtab") { symtabSection = static_cast<GElfSymbolTable*>(section.get()); }
        if (section->Name() == ".note") { noteSection = static_cast<GElfNoteSection*>(section.get()); }
      }

      size_t phnum;
      if (elf_getphdrnum(e, &phnum) < 0) { return elfError("elf_getphdrnum failed"); }
      for (size_t i = 0; i < phnum; ++i) {
        segments.push_back(std::unique_ptr<GElfSegment>(new GElfSegment(this, i)));
        if (!segments[i]->pull()) { return false; }
      }

      return true;
    }

    bool GElfImage::elfError(const char* msg)
    {
      out << "Error: " << msg << ": " << elfError() << std::endl;
      return false;
    }

    bool GElfImage::push0()
    {
      assert(e);
      for (std::unique_ptr<GElfSection>& section : sections) {
        if (!section->push()) { return false; }
      }
      for (std::unique_ptr<GElfSegment>& segment : segments) {
        if (!segment->push()) { return false; }
      }
      if (elf_update(e, ELF_C_NULL) < 0) { return elfError("elf_update (1) failed"); }
      return true;
    }

    bool GElfImage::push()
    {
      if (!push0()) { return false; }
      if (elf_update(e, ELF_C_WRITE) < 0) { return elfError("elf_update (2) failed"); }
      return true;
    }

    Segment* GElfImage::segmentByVAddr(uint64_t vaddr)
    {
      for (std::unique_ptr<GElfSegment>& seg : segments) {
        if (seg->vaddr() <= vaddr && vaddr < seg->vaddr() + seg->memSize()) {
          return seg.get();
        }
      }
      return 0;
    }

    bool GElfImage::elfEnd()
    {
      return false;
    }

    bool GElfImage::writeTo(const std::string& filename)
    {
      if (!push()) { return false; }
      if (!img.writeTo(filename)) { return imgError(); }
      return true;
    }

    GElfStringTable* GElfImage::addStringTable(const std::string& name) 
    {
      GElfStringTable* stab = new GElfStringTable(this);
      sections.push_back(std::unique_ptr<GElfStringTable>(stab));
      return stab;
    }

    GElfStringTable* GElfImage::getStringTable(uint16_t index)
    {
      return static_cast<GElfStringTable*>(sections[index].get());
    }

    GElfSymbolTable* GElfImage::addSymbolTable(const std::string& name, StringTable* stab)
    {
      if (!stab) { stab = strtab(); }
      const char* name0 = shstrtab()->addString(name);
      GElfSymbolTable* symtab = new GElfSymbolTable(this);
      symtab->push(name0, static_cast<GElfStringTable*>(stab));
      sections.push_back(std::unique_ptr<GElfSection>(symtab));
      return symtab;
    }

    GElfStringTable* GElfImage::shstrtab() {
      if (!shstrtabSection) {
        shstrtabSection = addStringTable(".shstrtab");
      }
      return shstrtabSection;
    }

    GElfStringTable* GElfImage::strtab() {
      if (!shstrtabSection) {
        strtabSection = addStringTable(".shstrtab");
      }
      return strtabSection;
    }

    GElfSymbolTable* GElfImage::symtab()
    {
      if (!symtabSection) {
        symtabSection = addSymbolTable(".symtab", strtab());
      }
      return symtabSection;
    }


    size_t GElfImage::reserveSegments(size_t count)
    {
      assert(segments.capacity() == 0);
      if (!gelf_newphdr(e, count)) { return elfError("gelf_newphdr failed"); }
      if (elf_update(e, ELF_C_NULL) < 0) { return elfError("elf_update (2) failed"); }
      segments.reserve(count);
      return count;
    }

    Segment* GElfImage::addSegment(uint32_t type, uint16_t vaddr, uint32_t flags, uint64_t align)
    {
      size_t index = segments.size();
      GElfSegment* segment = new GElfSegment(this, index);
      if (!segment->push(type, vaddr, flags, align)) { return 0; }
      segments.push_back(std::unique_ptr<GElfSegment>(segment));
      return segment;
    }

    GElfNoteSection* GElfImage::note()
    {
      if (!noteSection) { noteSection = addNoteSection(".note"); }
      return noteSection;
    }

    GElfNoteSection* GElfImage::addNoteSection(const std::string& name)
    {
      GElfNoteSection* note = new GElfNoteSection(this);
      note->push(name);
      sections.push_back(std::unique_ptr<GElfSection>(note));
      return note;
    }


    uint16_t GElfImage::machine() const
    {
      return ehdr.e_machine;
    }

    uint16_t GElfImage::etype() const
    {
      return ehdr.e_type;
    }

    Image* NewElf32Image() { return new GElfImage(ELFCLASS32); }
    Image* NewElf64Image() { return new GElfImage(ELFCLASS64); }

    uint64_t ElfSize(const void* emi)
    {
      const Elf64_Ehdr *ehdr = (const Elf64_Ehdr*) emi;
      if (NULL == ehdr || EV_CURRENT != ehdr->e_version) {
        return false;
      }

      const Elf64_Shdr *shdr = (const Elf64_Shdr*)((char*)emi + ehdr->e_shoff);
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

    std::string GetNoteString(uint32_t s_size, const char* s)
    {
      if (!s_size) { return ""; }
      if (s[s_size-1] == '\0') {
        return std::string(s, s_size-1);
      } else {
        return std::string(s, s_size);
      }
    }

  }
}
