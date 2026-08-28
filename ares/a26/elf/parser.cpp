namespace {

auto read16(std::span<const u8> data, u32 offset, u16& value) -> bool {
  if(offset > data.size() || 2 > data.size() - offset) return false;
  value = (u16)data[offset] | (u16)data[offset + 1] << 8;
  return true;
}

auto read32(std::span<const u8> data, u32 offset, u32& value) -> bool {
  if(offset > data.size() || 4 > data.size() - offset) return false;
  value = (u32)data[offset] | (u32)data[offset + 1] << 8
    | (u32)data[offset + 2] << 16 | (u32)data[offset + 3] << 24;
  return true;
}

auto bounded(u64 offset, u64 size, u64 capacity) -> bool {
  return offset <= capacity && size <= capacity - offset;
}

auto stringAt(std::span<const u8> table, u32 offset, string& output) -> bool {
  if(offset >= table.size()) return false;
  auto end = offset;
  while(end < table.size() && table[end]) end++;
  if(end == table.size()) return false;
  output = string{string_view{(const char*)table.data() + offset, end - offset}};
  return true;
}

}

auto Object::bytes(u32 offset, u32 size) const -> std::span<const u8> {
  if(!bounded(offset, size, image.size())) return {};
  return {image.data() + offset, size};
}

auto Object::parse(std::span<const u8> source) -> bool {
  image.clear();
  sections.clear();
  symbols.clear();
  relocations.clear();
  failure = {};
  parsed = false;
  auto fail = [&](const char* reason) { failure = reason; return false; };

  if(source.size() < 52) return fail("ELF header is truncated");
  if(source.size() > MaximumImageSize) return fail("ELF image exceeds 16 MiB");
  if(source[0] != 0x7f || source[1] != 'E' || source[2] != 'L' || source[3] != 'F') return fail("bad ELF magic");
  if(source[4] != 1) return fail("ELF is not 32-bit");
  if(source[5] != 1) return fail("ELF is not little-endian");
  if(source[6] != 1) return fail("bad ELF identification version");

  u16 type = 0, machine = 0, headerSize = 0, sectionSize = 0, sectionCount = 0, namesIndex = 0;
  u32 version = 0, sectionOffset = 0;
  if(!read16(source, 0x10, type) || !read16(source, 0x12, machine)
    || !read32(source, 0x14, version) || !read32(source, 0x20, sectionOffset)
    || !read16(source, 0x28, headerSize) || !read16(source, 0x2e, sectionSize)
    || !read16(source, 0x30, sectionCount) || !read16(source, 0x32, namesIndex)) {
    return fail("ELF header fields are truncated");
  }
  if(type != 1) return fail("ELF is not relocatable");
  if(machine != 40) return fail("ELF machine is not ARM");
  if(version != 1 || headerSize != 52) return fail("unsupported ELF header version or size");
  if(sectionSize != 40 || !sectionCount || sectionCount > 4096) return fail("invalid ELF section table shape");
  if(namesIndex >= sectionCount) return fail("section-name table index is out of range");
  if(sectionOffset < 52) return fail("section table overlaps the ELF header");
  if(!bounded(sectionOffset, (u64)sectionSize * sectionCount, source.size())) {
    return fail("section table exceeds ELF image");
  }

  image.assign(source.begin(), source.end());
  sections.resize(sectionCount);
  relocations.resize(sectionCount);
  for(u32 index = 0; index < sectionCount; index++) {
    auto offset = sectionOffset + index * sectionSize;
    auto& section = sections[index];
    if(!read32(source, offset + 0x00, section.nameOffset)
      || !read32(source, offset + 0x04, section.type)
      || !read32(source, offset + 0x08, section.flags)
      || !read32(source, offset + 0x0c, section.address)
      || !read32(source, offset + 0x10, section.offset)
      || !read32(source, offset + 0x14, section.size)
      || !read32(source, offset + 0x18, section.link)
      || !read32(source, offset + 0x1c, section.info)
      || !read32(source, offset + 0x20, section.alignment)
      || !read32(source, offset + 0x24, section.entrySize)) return fail("section header is truncated");
    if(section.type != SHT_NOBITS && !bounded(section.offset, section.size, source.size())) {
      return fail("section exceeds ELF image");
    }
    if(section.alignment && (section.alignment & (section.alignment - 1))) {
      return fail("section alignment is not a power of two");
    }
    if(section.alignment > 1_MiB) return fail("section alignment is excessive");
  }
  if(sections[0].type != SHT_NULL || sections[0].size) return fail("ELF section zero is not null");

  //Each file-backed section must own a distinct byte range.  Aliasing a
  //string/symbol/relocation payload makes later validation order-dependent
  //and is never emitted by the supported relocatable-object toolchain.
  for(u32 first = 1; first < sections.size(); first++) {
    auto& lhs = sections[first];
    if(lhs.type == SHT_NOBITS || !lhs.size) continue;
    if(lhs.offset < 52) return fail("section overlaps the ELF header");
    auto sectionTableEnd = (u64)sectionOffset + (u64)sectionSize * sectionCount;
    auto lhsEnd = (u64)lhs.offset + lhs.size;
    if(lhs.offset < sectionTableEnd && sectionOffset < lhsEnd) return fail("section overlaps the section table");
    for(u32 second = first + 1; second < sections.size(); second++) {
      auto& rhs = sections[second];
      if(rhs.type == SHT_NOBITS || !rhs.size) continue;
      auto rhsEnd = (u64)rhs.offset + rhs.size;
      if(lhs.offset < rhsEnd && rhs.offset < lhsEnd) return fail("file-backed ELF sections overlap");
    }
  }

  auto& names = sections[namesIndex];
  if(names.type != SHT_STRTAB) return fail("section-name table has the wrong type");
  auto nameBytes = bytes(names.offset, names.size);
  for(auto& section : sections) {
    if(!stringAt(nameBytes, section.nameOffset, section.name)) {
      return fail("section name is out of bounds or unterminated");
    }
  }

  u32 symbolSection = ~0u;
  for(u32 index = 0; index < sections.size(); index++) if(sections[index].type == SHT_SYMTAB) {
    if(symbolSection != ~0u) return fail("multiple symbol tables are unsupported");
    symbolSection = index;
  }
  if(symbolSection != ~0u) {
    auto& table = sections[symbolSection];
    if(table.entrySize != 16 || table.size % 16) return fail("invalid symbol table entry size");
    if(table.link >= sections.size() || sections[table.link].type != SHT_STRTAB) {
      return fail("symbol string table is invalid");
    }
    auto strings = bytes(sections[table.link].offset, sections[table.link].size);
    auto count = table.size / 16;
    if(count > 65536) return fail("symbol table is excessive");
    symbols.resize(count);
    for(u32 index = 0; index < count; index++) {
      auto offset = table.offset + index * 16;
      auto& symbol = symbols[index];
      if(!read32(source, offset + 0x00, symbol.nameOffset)
        || !read32(source, offset + 0x04, symbol.value)
        || !read32(source, offset + 0x08, symbol.size)
        || !read16(source, offset + 0x0e, symbol.section)) return fail("symbol entry is truncated");
      symbol.bind = source[offset + 0x0c] >> 4;
      symbol.type = source[offset + 0x0c] & 15;
      symbol.visibility = source[offset + 0x0d] & 3;
      if(symbol.type == 3 && symbol.section < sections.size()) symbol.name = sections[symbol.section].name;
      else if(!stringAt(strings, symbol.nameOffset, symbol.name)) {
        return fail("symbol name is out of bounds or unterminated");
      }
      if(symbol.section != SHN_UNDEF && symbol.section != SHN_ABS && symbol.section >= sections.size()) {
        return fail("symbol section is out of range");
      }
    }
  }

  for(u32 sectionIndex = 0; sectionIndex < sections.size(); sectionIndex++) {
    auto& table = sections[sectionIndex];
    if(table.type != SHT_REL && table.type != SHT_RELA) continue;
    auto entrySize = table.type == SHT_REL ? 8u : 12u;
    if(table.entrySize != entrySize || table.size % entrySize) return fail("invalid relocation table entry size");
    if(table.link != symbolSection || table.info >= sections.size()) {
      return fail("relocation table link or target is invalid");
    }
    auto count = table.size / entrySize;
    if(count > 262144) return fail("relocation table is excessive");
    auto& output = relocations[table.info];
    for(u32 index = 0; index < count; index++) {
      auto offset = table.offset + index * entrySize;
      u32 info = 0, addend = 0;
      Relocation relocation;
      if(!read32(source, offset, relocation.offset) || !read32(source, offset + 4, info)) {
        return fail("relocation entry is truncated");
      }
      if(table.type == SHT_RELA && !read32(source, offset + 8, addend)) return fail("relocation addend is truncated");
      relocation.symbol = info >> 8;
      relocation.type = info;
      relocation.explicitAddend = table.type == SHT_RELA;
      relocation.addend = (i32)addend;
      if(relocation.symbol >= symbols.size()) return fail("relocation symbol is out of range");
      output.push_back(relocation);
    }
  }
  parsed = true;
  return true;
}
