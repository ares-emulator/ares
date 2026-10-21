#pragma once
// Adapted from ares-64 1c9b9637 nall/decode/elf.hpp; bounded local capture metadata only.
#include <cstring>

#include <nall/string.hpp>
#include <vector>

namespace nall::Decode {

//Minimal ELF reader (ELF32/ELF64, little- or big-endian): section table,
//symbol table, and virtual-address -> file mapping. Operates directly on a
//caller-owned byte buffer (no copy is made); that buffer must outlive the ELF.

struct ELF {
  struct Section {
    string name;
    u32 type = 0;     //SHT_* (8 = SHT_NOBITS / .bss: no file backing)
    u64 flags = 0, addr = 0, offset = 0, size = 0, entsize = 0;
    u32 link = 0;
  };

  struct Symbol {
    string name;
    u64 value = 0, size = 0;
    u16 shndx = 0;
    u8  info = 0;
  };

  ELF() = default;
  ELF(const u8* data, u64 size) { load(data, size); }

  explicit operator bool() const { return _data; }

  auto load(const u8* data, u64 size) -> bool {
    _data = nullptr; _size = 0; _sections.clear();
    if(!data || size < 64) return false;
    if(data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') return false;
    u8 cls = data[4], enc = data[5];
    if(cls != 1 && cls != 2) return false;  //1 = ELF32, 2 = ELF64
    if(enc != 1 && enc != 2) return false;  //1 = LE,    2 = BE
    _data = data; _size = size; _is64 = (cls == 2); _bigEndian = (enc == 2);
    parseSections();
    if(_sections.empty()) { _data = nullptr; return false; }
    return true;
  }

  auto is64() const -> bool { return _is64; }
  auto bigEndian() const -> bool { return _bigEndian; }
  auto data() const -> const u8* { return _data; }
  auto size() const -> u64 { return _size; }

  //endian-aware fixed reads at an absolute file offset (bounds-checked)
  auto read16(u64 o) const -> u16 {
    if(o > _size || _size - o < 2) return 0;
    return _bigEndian ? (u16(_data[o]) << 8) | _data[o+1]
                      : (u16(_data[o+1]) << 8) | _data[o];
  }
  auto read32(u64 o) const -> u32 {
    if(o > _size || _size - o < 4) return 0;
    if(_bigEndian) return (u32(_data[o])<<24) | (u32(_data[o+1])<<16) | (u32(_data[o+2])<<8) | _data[o+3];
    return (u32(_data[o+3])<<24) | (u32(_data[o+2])<<16) | (u32(_data[o+1])<<8) | _data[o];
  }
  auto read64(u64 o) const -> u64 {
    if(o > _size || _size - o < 8) return 0;
    u64 v = 0;
    if(_bigEndian) for(u32 i = 0; i < 8; i++) v = (v << 8) | _data[o + i];
    else           for(s32 i = 7; i >= 0; i--) v = (v << 8) | _data[o + i];
    return v;
  }
  //a native pointer-sized value (4 bytes on ELF32, 8 on ELF64)
  auto readAddr(u64 o) const -> u64 { return _is64 ? read64(o) : read32(o); }

  auto sections() const -> const std::vector<Section>& { return _sections; }

  auto section(const string& name) const -> const Section* {
    for(auto& s : _sections) if(s.name == name) return &s;
    return nullptr;
  }

  //pointer to a section's bytes and its readable size; null if absent
  auto sectionData(const string& name, u64& outSize) const -> const u8* {
    outSize = 0;
    auto* s = section(name);
    if(!s || s->offset >= _size) return nullptr;
    outSize = min(s->size, _size - s->offset);
    return _data + s->offset;
  }

  //all entries of .symtab (names resolved through .strtab)
  auto symbols() const -> std::vector<Symbol> {
    std::vector<Symbol> out;
    auto* symtab = section(".symtab");
    auto* strtab = section(".strtab");
    if(!symtab || !strtab || symtab->entsize < (_is64 ? 24 : 16)) return out;
    u32 valOff = _is64 ? 8 : 4;
    for(u64 s = 0; s < symtab->size && symtab->entsize <= symtab->size - s && out.size() < 65536; s += symtab->entsize) {
      u64 base = symtab->offset + s;
      if(base + symtab->entsize > _size) break;
      Symbol sym;
      auto nameOffset = read32(base);
      if(nameOffset >= strtab->size) continue;
      sym.name = cString(strtab->offset + nameOffset);
      sym.value = readAddr(base + valOff);
      if(_is64) { sym.info = _data[base + 4];  sym.shndx = read16(base + 6);  sym.size = read64(base + 16); }
      else      { sym.info = _data[base + 12]; sym.shndx = read16(base + 14); sym.size = read32(base + 8); }
      out.push_back(std::move(sym));
    }
    return out;
  }

  //look up a single symbol's value by name
  auto symbolValue(const string& name, u64& value) const -> bool {
    for(auto& sym : symbols()) if(sym.name == name) { value = sym.value; return true; }
    return false;
  }

  //map a virtual address (sh_addr space) to a file pointer; avail = bytes left
  auto virtualAddress(u64 vaddr, u64& avail) const -> const u8* {
    avail = 0;
    for(auto& s : _sections) {
      if(s.type == 8) continue;  //SHT_NOBITS: not backed by file data
      if(!s.addr) continue;
      if(vaddr >= s.addr && vaddr - s.addr < s.size) {
        u64 fo = s.offset + (vaddr - s.addr);
        if(fo >= _size) return nullptr;
        avail = min(s.size - (vaddr - s.addr), _size - fo);
        return _data + fo;
      }
    }
    return nullptr;
  }

  //read an endian-correct 32-bit word at a virtual address
  auto readWord(u64 vaddr, u32& out) const -> bool {
    u64 avail = 0;
    const u8* p = virtualAddress(vaddr, avail);
    if(!p || avail < 4) return false;
    out = _bigEndian ? (u32(p[0])<<24) | (u32(p[1])<<16) | (u32(p[2])<<8) | p[3]
                     : (u32(p[3])<<24) | (u32(p[2])<<16) | (u32(p[1])<<8) | p[0];
    return true;
  }

private:
  //NUL-terminated string at an absolute file offset
  auto cString(u64 offset) const -> string {
    if(!offset || offset >= _size) return {};
    auto end = (const u8*)std::memchr(_data + offset, 0, std::min<u64>(256, _size - offset));
    if(!end) return {};
    return string{(const char*)(_data + offset)};
  }

  auto parseSections() -> void {
    _sections.clear();
    u64 shoff; u32 shentsize, shnum, shstrndx, shdrSize, shFlagsOff, shAddrOff, shOffOff, shSizeOff;
    if(_is64) {
      shoff = read64(40); shentsize = read16(58); shnum = read16(60); shstrndx = read16(62);
      shdrSize = 64; shFlagsOff = 8; shAddrOff = 16; shOffOff = 24; shSizeOff = 32;
    } else {
      shoff = read32(32); shentsize = read16(46); shnum = read16(48); shstrndx = read16(50);
      shdrSize = 40; shFlagsOff = 8; shAddrOff = 12; shOffOff = 16; shSizeOff = 20;
    }
    if(!shoff || !shnum || shnum > 4096 || shentsize < shdrSize || shstrndx >= shnum || shoff > _size || u64(shnum) * shentsize > _size - shoff) return;
    //offset of the section-header string table (for resolving names)
    u64 shstrOff = 0;
    { u64 h = shoff + (u64)shstrndx * shentsize;
      if(h + shdrSize <= _size) shstrOff = readAddr(h + shOffOff); }
    for(u32 i = 0; i < shnum; i++) {
      u64 h = shoff + (u64)i * shentsize;
      if(h + shdrSize > _size) break;
      Section s;
      u32 nameIdx = read32(h);
      s.type    = read32(h + 4);
      s.flags   = readAddr(h + shFlagsOff);
      s.addr    = readAddr(h + shAddrOff);
      s.offset  = readAddr(h + shOffOff);
      s.size    = readAddr(h + shSizeOff);
      s.link    = read32(h + (_is64 ? 40 : 24));
      s.entsize = readAddr(h + (_is64 ? 56 : 36));
      if(s.type != 8 && (s.offset > _size || s.size > _size - s.offset)) { _sections.clear(); return; }
      if(s.addr > UINT64_MAX - s.size) { _sections.clear(); return; }
      if(shstrOff < _size && nameIdx < _size - shstrOff) s.name = cString(shstrOff + nameIdx);
      _sections.push_back(std::move(s));
    }
  }

  const u8* _data = nullptr;
  u64 _size = 0;
  bool _is64 = false;
  bool _bigEndian = false;
  std::vector<Section> _sections;
};

}
