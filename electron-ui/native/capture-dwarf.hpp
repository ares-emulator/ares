#pragma once

#include "capture-elf.hpp"
#include <stdexcept>
// Adapted from ares-64 1c9b9637; malformed metadata is bounded and discarded.
#include <vector>
#include <unordered_map>

namespace nall::Decode {

//Minimal DWARF (v2-v5, 32-bit, endian per the ELF) reader. Just enough to
//locate a structure type by name and flatten its members -- recursively through
//nested structs/unions and arrays -- into (offset, size, stride, name) leaves.
//This is handy for mapping a byte offset within a C struct back to a field name
//(e.g. a memory-mapped layout described by a struct in the program's DWARF).

struct DWARF {
  struct Field {
    u64 offset = 0;   //byte offset of this leaf within the struct
    u64 size = 0;     //bytes the field/array occupies
    u64 stride = 0;   //element size when this is an array leaf (0 => scalar)
    string name;      //dotted member path, e.g. "rdp_mode.combiner"
  };

  DWARF() = default;
  DWARF(const ELF& elf) { load(elf); }

  explicit operator bool() const { return info && abbrev; }

  auto load(const ELF& elf) -> bool {
    bigEndian = elf.bigEndian();
    u64 ai = 0, aa = 0, as = 0, al = 0;
    info    = elf.sectionData(".debug_info",     ai);
    abbrev  = elf.sectionData(".debug_abbrev",   aa);
    str     = elf.sectionData(".debug_str",      as);
    lineStr = elf.sectionData(".debug_line_str", al);
    infoSize = (u32)ai; abbrevSize = (u32)aa; strSize = (u32)as; lineStrSize = (u32)al;
    return info && abbrev;
  }

  //Find struct `typeName` and return its flattened leaf fields ({} if absent).
  auto flattenStruct(const string& typeName) -> std::vector<Field> {
    out.clear();
    want = typeName;
    fuel = 2000000;
    try { if(info && abbrev) run(); } catch(const std::exception&) { out.clear(); }
    return std::move(out);
  }

private:
  enum {
    DW_TAG_array_type = 0x01, DW_TAG_member = 0x0d, DW_TAG_pointer_type = 0x0f,
    DW_TAG_structure_type = 0x13, DW_TAG_typedef = 0x16, DW_TAG_union_type = 0x17,
    DW_TAG_subrange_type = 0x21, DW_TAG_const_type = 0x26, DW_TAG_volatile_type = 0x35,
    DW_TAG_restrict_type = 0x37, DW_TAG_atomic_type = 0x47,
  };
  enum {
    DW_AT_name = 0x03, DW_AT_byte_size = 0x0b, DW_AT_upper_bound = 0x2f,
    DW_AT_count = 0x37, DW_AT_data_member_location = 0x38, DW_AT_type = 0x49,
  };
  enum {
    DW_FORM_addr = 0x01, DW_FORM_block2 = 0x03, DW_FORM_block4 = 0x04, DW_FORM_data2 = 0x05,
    DW_FORM_data4 = 0x06, DW_FORM_data8 = 0x07, DW_FORM_string = 0x08, DW_FORM_block = 0x09,
    DW_FORM_block1 = 0x0a, DW_FORM_data1 = 0x0b, DW_FORM_flag = 0x0c, DW_FORM_sdata = 0x0d,
    DW_FORM_strp = 0x0e, DW_FORM_udata = 0x0f, DW_FORM_ref_addr = 0x10, DW_FORM_ref1 = 0x11,
    DW_FORM_ref2 = 0x12, DW_FORM_ref4 = 0x13, DW_FORM_ref8 = 0x14, DW_FORM_ref_udata = 0x15,
    DW_FORM_indirect = 0x16, DW_FORM_sec_offset = 0x17, DW_FORM_exprloc = 0x18,
    DW_FORM_flag_present = 0x19, DW_FORM_strx = 0x1a, DW_FORM_addrx = 0x1b, DW_FORM_ref_sup4 = 0x1c,
    DW_FORM_strp_sup = 0x1d, DW_FORM_data16 = 0x1e, DW_FORM_line_strp = 0x1f, DW_FORM_ref_sig8 = 0x20,
    DW_FORM_implicit_const = 0x21, DW_FORM_loclistx = 0x22, DW_FORM_rnglistx = 0x23,
    DW_FORM_ref_sup8 = 0x24, DW_FORM_strx1 = 0x25, DW_FORM_strx2 = 0x26, DW_FORM_strx3 = 0x27,
    DW_FORM_strx4 = 0x28, DW_FORM_addrx1 = 0x29, DW_FORM_addrx2 = 0x2a, DW_FORM_addrx3 = 0x2b,
    DW_FORM_addrx4 = 0x2c,
  };
  enum { DW_OP_plus_uconst = 0x23 };

  //endian-aware fixed reads over a section buffer (LEB128 is endian-independent)
  auto rd16(const u8* d, u32 p) const -> u16 {
    if(d != info || p > infoSize || infoSize-p < 2) throw std::runtime_error("truncated DWARF");
    return bigEndian ? (u16(d[p]) << 8) | d[p+1] : (u16(d[p+1]) << 8) | d[p];
  }
  auto rd32(const u8* d, u32 p) const -> u32 {
    if(d != info || p > infoSize || infoSize-p < 4) throw std::runtime_error("truncated DWARF");
    if(bigEndian) return (u32(d[p])<<24) | (u32(d[p+1])<<16) | (u32(d[p+2])<<8) | d[p+3];
    return (u32(d[p+3])<<24) | (u32(d[p+2])<<16) | (u32(d[p+1])<<8) | d[p];
  }
  auto rd64(const u8* d, u32 p) const -> u64 {
    if(d != info || p > infoSize || infoSize-p < 8) throw std::runtime_error("truncated DWARF");
    u64 v = 0;
    if(bigEndian) for(u32 i = 0; i < 8; i++) v = (v << 8) | d[p + i];
    else          for(s32 i = 7; i >= 0; i--) v = (v << 8) | d[p + i];
    return v;
  }
  static auto readUleb(const u8* d, u32 size, u32& p) -> u64 {
    u64 r = 0; u32 s = 0;
    while(p < size) { u8 b = d[p++]; r |= (u64)(b & 0x7f) << s; if(!(b & 0x80)) break; s += 7; if(s >= 64) break; }
    return r;
  }
  static auto readSleb(const u8* d, u32 size, u32& p) -> s64 {
    s64 r = 0; u32 s = 0; u8 b = 0;
    while(p < size && s < 64) { b = d[p++]; r |= (u64)(b & 0x7f) << s; s += 7; if(!(b & 0x80)) break; }
    if(s < 64 && (b & 0x40)) r |= static_cast<s64>(~u64(0) << s);
    return r;
  }

  u64 fuel = 2000000;
  auto tick() -> void { if(!fuel--) throw std::runtime_error("DWARF work limit"); }
  auto byte(u32 p) const -> u8 { if(p >= infoSize) throw std::runtime_error("truncated DWARF"); return info[p]; }
  static auto safeString(const u8* d, u32 size, u32 p) -> const char* {
    if(p >= size || !std::memchr(d+p,0,std::min<u32>(256,size-p))) return nullptr;
    return (const char*)(d+p);
  }
  struct AttrSpec { u32 attr; u32 form; s64 implicit; };
  struct Abbrev { u32 tag = 0; bool children = false; std::vector<AttrSpec> attrs; };

  //one decoded DIE -- only the attributes we care about are captured
  struct Die {
    u64 code = 0;
    u32 tag = 0;
    bool children = false;
    u32 nextOff = 0;            //offset just past this DIE's attributes
    const char* name = nullptr;
    bool hasByteSize = false; u64 byteSize = 0;
    bool hasMemberLoc = false; u64 memberLoc = 0;
    bool hasType = false; u32 typeRef = 0;   //absolute .debug_info offset
    bool hasUpper = false; u64 upperBound = 0;
    bool hasCount = false; u64 count = 0;
  };

  auto parseAbbrev(u32 offset) -> void {
    abbrevMap.clear();
    u32 p = offset;
    while(p < abbrevSize) {
      tick();
      u64 code = readUleb(abbrev, abbrevSize, p);
      if(code == 0) break;  //end of this table
      Abbrev a;
      a.tag = (u32)readUleb(abbrev, abbrevSize, p);
      a.children = p < abbrevSize ? abbrev[p++] != 0 : false;
      for(;;) {
        tick();
        u32 at = (u32)readUleb(abbrev, abbrevSize, p);
        u32 fm = (u32)readUleb(abbrev, abbrevSize, p);
        if(at == 0 && fm == 0) break;
        s64 ic = (fm == DW_FORM_implicit_const) ? readSleb(abbrev, abbrevSize, p) : 0;
        a.attrs.push_back({at, fm, ic});
      }
      abbrevMap[code] = std::move(a);
    }
  }

  //advance `p` past one attribute value; expose its numeric value (val), any
  //string pointer (sval), and block contents (bptr/blen) for callers
  auto readForm(u32& p, u32 form, s64 implicit,
                u64& val, const char*& sval, const u8*& bptr, u32& blen) -> void {
    tick();
    val = 0; sval = nullptr; bptr = nullptr; blen = 0;
    switch(form) {
      case DW_FORM_addr:
        for(u32 i = 0; i < addrSize && p < infoSize; i++) val = (val << 8) | byte(p++);
        break;
      case DW_FORM_data1: case DW_FORM_ref1: case DW_FORM_flag:
      case DW_FORM_strx1: case DW_FORM_addrx1:
        val = byte(p); p += 1; break;
      case DW_FORM_data2: case DW_FORM_ref2: case DW_FORM_strx2: case DW_FORM_addrx2:
        val = rd16(info, p); p += 2; break;
      case DW_FORM_strx3: case DW_FORM_addrx3:
        val = ((u32)byte(p) << 16) | ((u32)byte(p+1) << 8) | byte(p+2); p += 3; break;
      case DW_FORM_data4: case DW_FORM_ref4: case DW_FORM_sec_offset:
      case DW_FORM_ref_addr: case DW_FORM_strx4: case DW_FORM_addrx4: case DW_FORM_ref_sup4:
        val = rd32(info, p); p += 4; break;
      case DW_FORM_data8: case DW_FORM_ref8: case DW_FORM_ref_sig8: case DW_FORM_ref_sup8:
        val = rd64(info, p); p += 8; break;
      case DW_FORM_data16: p += 16; break;
      case DW_FORM_sdata: val = (u64)readSleb(info, infoSize, p); break;
      case DW_FORM_udata: case DW_FORM_ref_udata: case DW_FORM_strx:
      case DW_FORM_addrx: case DW_FORM_loclistx: case DW_FORM_rnglistx:
        val = readUleb(info, infoSize, p); break;
      case DW_FORM_string:
        sval = safeString(info,infoSize,p);
        while(p < infoSize && info[p]) p++;
        p++;  //skip NUL
        break;
      case DW_FORM_strp: {
        u32 o = rd32(info, p); p += 4; val = o;
        if(str) sval = safeString(str,strSize,o);
      } break;
      case DW_FORM_line_strp: {
        u32 o = rd32(info, p); p += 4; val = o;
        if(lineStr) sval = safeString(lineStr,lineStrSize,o);
      } break;
      case DW_FORM_strp_sup: p += 4; break;
      case DW_FORM_block1: { blen = byte(p); p += 1; if(p > infoSize || blen > infoSize-p) throw std::runtime_error("truncated DWARF block"); bptr = info + p; p += blen; } break;
      case DW_FORM_block2: { blen = rd16(info, p); p += 2; if(p > infoSize || blen > infoSize-p) throw std::runtime_error("truncated DWARF block"); bptr = info + p; p += blen; } break;
      case DW_FORM_block4: { blen = rd32(info, p); p += 4; if(p > infoSize || blen > infoSize-p) throw std::runtime_error("truncated DWARF block"); bptr = info + p; p += blen; } break;
      case DW_FORM_block: case DW_FORM_exprloc:
        { blen = (u32)readUleb(info, infoSize, p); if(p > infoSize || blen > infoSize-p) throw std::runtime_error("truncated DWARF block"); bptr = info + p; p += blen; } break;
      case DW_FORM_flag_present: val = 1; break;
      case DW_FORM_implicit_const: val = (u64)implicit; break;
      case DW_FORM_indirect: {
        u32 f2 = (u32)readUleb(info, infoSize, p);
        if(f2 == DW_FORM_indirect) throw std::runtime_error("nested indirect DWARF");
        readForm(p, f2, 0, val, sval, bptr, blen);
      } break;
      default: throw std::runtime_error("unsupported DWARF form");
    }
  }

  static auto isRefForm(u32 form) -> bool {
    return form == DW_FORM_ref1 || form == DW_FORM_ref2 || form == DW_FORM_ref4
        || form == DW_FORM_ref8 || form == DW_FORM_ref_udata;
  }

  auto parseDie(u32 off) -> Die {
    tick();
    Die d; d.nextOff = off;
    if(off >= infoSize) return d;
    u32 p = off;
    d.code = readUleb(info, infoSize, p);
    if(d.code == 0) { d.nextOff = p; return d; }  //null DIE terminator
    auto it = abbrevMap.find(d.code);
    if(it == abbrevMap.end()) { d.nextOff = p; return d; }
    const Abbrev& a = it->second;
    d.tag = a.tag; d.children = a.children;
    for(const auto& spec : a.attrs) {
      u64 val; const char* sval; const u8* bptr; u32 blen;
      readForm(p, spec.form, spec.implicit, val, sval, bptr, blen);
      if(p > infoSize || (bptr && (bptr < info || uint64_t(bptr-info) > infoSize || blen > infoSize-uint64_t(bptr-info)))) throw std::runtime_error("truncated DWARF block");
      switch(spec.attr) {
        case DW_AT_name: if(sval) d.name = sval; break;
        case DW_AT_byte_size: d.hasByteSize = true; d.byteSize = val; break;
        case DW_AT_data_member_location:
          if(bptr) {  //exprloc: handle the common "DW_OP_plus_uconst <off>"
            if(blen >= 1 && bptr[0] == DW_OP_plus_uconst) {
              u32 bp = 1; d.memberLoc = readUleb(bptr, blen, bp); d.hasMemberLoc = true;
            }
          } else { d.hasMemberLoc = true; d.memberLoc = val; }
          break;
        case DW_AT_type:
          d.hasType = true;
          d.typeRef = isRefForm(spec.form) ? (u32)(cuStart + val) : (u32)val;
          break;
        case DW_AT_upper_bound: d.hasUpper = true; d.upperBound = val; break;
        case DW_AT_count: d.hasCount = true; d.count = val; break;
        default: break;
      }
    }
    if(p > infoSize || p <= off) throw std::runtime_error("truncated DWARF DIE");
    d.nextOff = p;
    return d;
  }

  //offset of the next sibling DIE at `off`, skipping any nested children
  auto skipSubtree(u32 off, u32 depth = 0) -> u32 {
    if(depth > 64) return cuEnd;
    Die d = parseDie(off);
    if(d.code == 0) return d.nextOff;
    u32 n = d.nextOff;
    if(d.children) {
      for(;;) {
        if(n >= cuEnd) break;
        Die e = parseDie(n);
        if(e.code == 0) { n = e.nextOff; break; }
        n = skipSubtree(n, depth + 1);
      }
    }
    return n;
  }

  //strip typedef/const/volatile/restrict/atomic to the underlying type DIE
  auto resolveType(u32 typeRef, u32 depth = 0) -> Die {
    Die d;
    if(typeRef == 0 || typeRef >= infoSize || depth > 32) return d;
    d = parseDie(typeRef);
    while((d.tag == DW_TAG_typedef || d.tag == DW_TAG_const_type
        || d.tag == DW_TAG_volatile_type || d.tag == DW_TAG_restrict_type
        || d.tag == DW_TAG_atomic_type) && d.hasType && depth < 32) {
      d = parseDie(d.typeRef); depth++;
    }
    return d;
  }

  //multiply the subrange counts of an array_type DIE; 0 if unknown
  auto arrayCount(const Die& arr) -> u32 {
    u32 total = 1; bool any = false; u32 o = arr.nextOff;
    while(o < cuEnd) {
      Die c = parseDie(o);
      if(c.code == 0) break;
      if(c.tag == DW_TAG_subrange_type) {
        u32 n = 0;
        if(c.hasCount) n = (u32)c.count;
        else if(c.hasUpper) n = (u32)c.upperBound + 1;
        if(n) { total *= n; any = true; }
      }
      o = skipSubtree(o);
    }
    return any ? total : 0;
  }

  auto typeSize(u32 typeRef, u32 depth = 0) -> u32 {
    if(depth > 32) return 0;
    Die d = resolveType(typeRef, depth);
    if(d.tag == DW_TAG_pointer_type) return addrSize;
    if(d.hasByteSize) return (u32)d.byteSize;
    if(d.tag == DW_TAG_array_type && d.hasType) {
      u32 es = typeSize(d.typeRef, depth + 1);
      u32 c = arrayCount(d);
      return es * (c ? c : 1);
    }
    return 0;
  }

  auto pushLeaf(u32 offset, u32 size, u32 stride, const string& name) -> void {
    if(out.size() >= 4096) return;
    out.push_back({offset, size ? size : 1, stride, name});
  }

  //flatten a type at byte offset `base` into leaf fields under `prefix`
  auto flatten(u32 typeRef, u32 base, const string& prefix, u32 depth) -> void {
    if(depth > 24 || out.size() >= 4096) return;
    Die t = resolveType(typeRef, depth);
    if(t.tag == DW_TAG_structure_type || t.tag == DW_TAG_union_type) {
      u32 o = t.nextOff;
      while(o < cuEnd) {
        Die m = parseDie(o);
        if(m.code == 0) break;
        if(m.tag == DW_TAG_member && m.name && m.hasType && m.hasMemberLoc) {
          string childName = prefix ? string{prefix, ".", m.name} : string{m.name};
          flatten(m.typeRef, base + (u32)m.memberLoc, childName, depth + 1);
        }
        o = skipSubtree(o);
      }
      return;
    }
    if(t.tag == DW_TAG_array_type && t.hasType) {
      u32 elemRef = t.typeRef;
      Die et = resolveType(elemRef, depth);
      u32 elemSize = typeSize(elemRef, depth); if(!elemSize) elemSize = 1;
      u32 count = arrayCount(t);
      if(!count && t.hasByteSize) count = (u32)t.byteSize / elemSize;
      bool elemAggregate = (et.tag == DW_TAG_structure_type || et.tag == DW_TAG_union_type);
      if(elemAggregate && count && count <= 64) {
        for(u32 i = 0; i < count; i++)
          flatten(elemRef, base + i * elemSize, string{prefix, "[", i, "]"}, depth + 1);
      } else {
        u32 total = t.hasByteSize ? (u32)t.byteSize : count * elemSize;
        pushLeaf(base, total ? total : elemSize, elemSize, prefix);
      }
      return;
    }
    //scalar leaf (base/pointer/enum/unknown)
    u32 sz = typeSize(typeRef, depth); if(!sz) sz = addrSize;
    pushLeaf(base, sz, 0, prefix);
  }

  //scan every CU for the wanted struct and flatten the first real definition
  auto run() -> bool {
    u32 cu = 0;
    while(cu <= infoSize && infoSize-cu >= 11) {
      tick();
      cuStart = cu;
      u32 len = rd32(info, cu);
      if(len == 0xFFFFFFFF || len == 0) break;  //64-bit DWARF unsupported / done
      if(len > infoSize-cu-4 || len < 7) throw std::runtime_error("truncated DWARF CU");
      cuEnd = cu + 4 + len;
      u16 ver = rd16(info, cu + 4);
      u32 p = cu + 6; u32 abbrevOff;
      if(ver >= 5) {
        p += 1;                 //unit_type
        addrSize = byte(p++);   //address_size
        abbrevOff = rd32(info, p); p += 4;
      } else {
        abbrevOff = rd32(info, p); p += 4;
        addrSize = byte(p++);
      }
      parseAbbrev(abbrevOff);

      for(u32 off = p; off < cuEnd && off < infoSize;) {
        Die d = parseDie(off);
        if(d.code == 0) { off = d.nextOff; continue; }
        if(d.name && want == d.name) {
          //match either a tagged struct/union directly, or a typedef naming an
          //anonymous one (e.g. `typedef struct { ... } rsp_ucode_t;`). flatten()
          //resolves typedefs, so passing the typedef's own type ref works.
          if(d.tag == DW_TAG_structure_type && d.children && d.hasByteSize && d.byteSize > 0) {
            flatten(off, 0, string{}, 0);
            if(!out.empty()) return true;
          } else if(d.tag == DW_TAG_typedef && d.hasType) {
            flatten(d.typeRef, 0, string{}, 0);
            if(!out.empty()) return true;
          }
        }
        off = d.nextOff;
      }
      cu = cuEnd;
    }
    return false;
  }

  const u8* info = nullptr; u32 infoSize = 0;
  const u8* abbrev = nullptr; u32 abbrevSize = 0;
  const u8* str = nullptr; u32 strSize = 0;
  const u8* lineStr = nullptr; u32 lineStrSize = 0;
  bool bigEndian = true;
  u8 addrSize = 4; u32 cuStart = 0; u32 cuEnd = 0;
  std::unordered_map<u64, Abbrev> abbrevMap;
  string want;
  std::vector<Field> out;
};

}
