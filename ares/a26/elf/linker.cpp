namespace {

auto alignTo(u32 value, u32 alignment, u32& output) -> bool {
  alignment = max(1u, alignment);
  auto mask = alignment - 1;
  if(value > ~0u - mask) return false;
  output = value + mask & ~mask;
  return true;
}

auto get32(const std::vector<u8>& bytes, u32 offset, u32& value) -> bool {
  if(offset > bytes.size() || 4 > bytes.size() - offset) return false;
  value = (u32)bytes[offset] | (u32)bytes[offset + 1] << 8
    | (u32)bytes[offset + 2] << 16 | (u32)bytes[offset + 3] << 24;
  return true;
}

auto put32(std::vector<u8>& bytes, u32 offset, u32 value) -> bool {
  if(offset > bytes.size() || 4 > bytes.size() - offset) return false;
  for(u32 byte = 0; byte < 4; byte++) bytes[offset + byte] = value >> byte * 8;
  return true;
}

auto get16(const std::vector<u8>& bytes, u32 offset, u16& value) -> bool {
  if(offset > bytes.size() || 2 > bytes.size() - offset) return false;
  value = bytes[offset] | bytes[offset + 1] << 8;
  return true;
}

auto put16(std::vector<u8>& bytes, u32 offset, u16 value) -> bool {
  if(offset > bytes.size() || 2 > bytes.size() - offset) return false;
  bytes[offset] = value;
  bytes[offset + 1] = value >> 8;
  return true;
}

auto signExtend(u32 value, u32 bits) -> i32 {
  auto shift = 32 - bits;
  return (i32)(value << shift) >> shift;
}

}

auto Linker::link(const Object& object, std::span<const External> externals) -> bool {
  Result candidate;
  failure = {};
  auto fail = [&](const char* reason) { failure = reason; return false; };
  if(!object.valid()) return fail("cannot link an invalid ELF object");
  candidate.placements.resize(object.sections.size());

  auto segmentFor = [](const Section& section) -> Segment {
    if(section.flags & SHF_EXECINSTR) return Segment::Text;
    if(section.flags & SHF_WRITE) return Segment::Data;
    return Segment::Rodata;
  };
  auto segment = [&](Segment type) -> std::vector<u8>& {
    if(type == Segment::Text) return candidate.text;
    if(type == Segment::Data) return candidate.data;
    return candidate.rodata;
  };
  auto base = [](Segment type) -> u32 {
    if(type == Segment::Text) return TextBase;
    if(type == Segment::Data) return DataBase;
    return RodataBase;
  };
  auto capacity = [](Segment type) -> u32 {
    if(type == Segment::Text) return TextCapacity;
    if(type == Segment::Data) return DataCapacity;
    return RodataCapacity;
  };

  for(u32 index = 0; index < object.sections.size(); index++) {
    auto& section = object.sections[index];
    if(!(section.flags & SHF_ALLOC)) continue;
    if(section.type != SHT_PROGBITS && section.type != SHT_NOBITS
      && section.type != SHT_INIT_ARRAY && section.type != SHT_PREINIT_ARRAY) continue;
    auto type = segmentFor(section);
    auto& output = segment(type);
    u32 offset = 0;
    if(!alignTo(output.size(), section.alignment, offset)) return fail("section layout overflow");
    if(offset > capacity(type) || section.size > capacity(type) - offset) {
      return fail("linked segment exceeds fixed capacity");
    }
    output.resize(offset + section.size, 0);
    if(section.type != SHT_NOBITS) {
      auto source = object.bytes(section.offset, section.size);
      if(source.size() != section.size) return fail("section copy exceeds ELF image");
      std::copy(source.begin(), source.end(), output.begin() + offset);
    }
    candidate.placements[index] = {true, type, offset};
  }

  candidate.relocatedSymbols.resize(object.symbols.size());
  for(u32 index = 0; index < object.symbols.size(); index++) {
    auto& symbol = object.symbols[index];
    u32 value = 0;
    if(symbol.section == SHN_UNDEF) {
      bool resolved = !symbol.name;
      for(auto& external : externals) if(external.name == symbol.name) {
        value = external.value;
        resolved = true;
        break;
      }
      if(!resolved && symbol.bind == 2) resolved = true;  //ELF weak undefined symbol.
      if(!resolved) { failure = {"undefined ELF symbol: ", symbol.name}; return false; }
    } else if(symbol.section == SHN_ABS) {
      value = symbol.value;
    } else {
      auto& placement = candidate.placements[symbol.section];
      auto& section = object.sections[symbol.section];
      auto symbolOffset = symbol.type == 2 ? symbol.value & ~1u : symbol.value;
      if(!placement.allocated || symbolOffset > section.size) return fail("symbol refers outside an allocated section");
      value = base(placement.segment) + placement.offset;
      if(symbol.type != 3) value += symbol.value;  //STT_SECTION names the section base.
    }
    if(symbol.type == 2) value |= 1;  //STT_FUNC is a Thumb entrypoint.
    candidate.relocatedSymbols[index] = value;
    if(symbol.name == "elf_main") {
      if(symbol.section == SHN_UNDEF || symbol.section == SHN_ABS || symbol.type != 2) {
        return fail("elf_main is not an allocated function");
      }
      auto& placement = candidate.placements[symbol.section];
      auto& section = object.sections[symbol.section];
      if(placement.segment != Segment::Text || (symbol.value & ~1u) >= section.size) {
        return fail("elf_main is outside executable text");
      }
      candidate.entrypoint = value;
    }
  }
  if(!candidate.entrypoint) return fail("required elf_main symbol is missing");

  for(u32 sectionIndex = 0; sectionIndex < object.relocations.size(); sectionIndex++) {
    auto& placement = candidate.placements[sectionIndex];
    if(object.relocations[sectionIndex].empty()) continue;
    if(!placement.allocated) return fail("relocation targets an unallocated section");
    auto& targetSection = object.sections[sectionIndex];
    auto& output = segment(placement.segment);
    auto sectionAddress = base(placement.segment) + placement.offset;
    for(auto& relocation : object.relocations[sectionIndex]) {
      if(relocation.symbol >= candidate.relocatedSymbols.size()) return fail("relocation symbol is out of range");
      if(relocation.offset > targetSection.size || 4 > targetSection.size - relocation.offset) {
        return fail("relocation exceeds its target section");
      }
      auto location = placement.offset + relocation.offset;
      auto place = sectionAddress + relocation.offset;
      auto symbol = candidate.relocatedSymbols[relocation.symbol];
      if(relocation.type == R_ARM_ABS32 || relocation.type == R_ARM_TARGET1 || relocation.type == R_ARM_REL32) {
        if(relocation.offset & 3) return fail("word relocation is not aligned");
        u32 encoded = 0;
        if(!get32(output, location, encoded)) return fail("word relocation exceeds its target section");
        auto addend = relocation.explicitAddend ? relocation.addend : (i32)encoded;
        auto value = relocation.type == R_ARM_REL32 ? symbol + addend - place : symbol + addend;
        if(!put32(output, location, value)) return fail("word relocation write failed");
        continue;
      }
      if(relocation.type == R_ARM_THM_CALL || relocation.type == R_ARM_THM_JUMP24) {
        if(relocation.offset & 1) return fail("Thumb relocation is not aligned");
        u16 first = 0, second = 0;
        if(!get16(output, location, first) || !get16(output, location + 2, second)) {
          return fail("Thumb relocation exceeds its target section");
        }
        i32 addend = relocation.addend;
        if(!relocation.explicitAddend) {
          auto sign = first >> 10 & 1;
          auto j1 = second >> 13 & 1;
          auto j2 = second >> 11 & 1;
          auto i1 = !(j1 ^ sign);
          auto i2 = !(j2 ^ sign);
          auto encoded = sign << 24 | i1 << 23 | i2 << 22
            | (first & 0x03ff) << 12 | (second & 0x07ff) << 1;
          addend = signExtend(encoded, 25);
        }
        auto displacement = (i64)(symbol & ~1u) + addend - place;
        if(relocation.explicitAddend) displacement -= 4;
        if(displacement & 1 || displacement < -0x0100'0000ll || displacement > 0x00ff'fffell) {
          return fail("Thumb branch relocation is out of range");
        }
        auto encoded = (u32)displacement & 0x01ff'ffff;
        auto sign = encoded >> 24 & 1;
        auto i1 = encoded >> 23 & 1;
        auto i2 = encoded >> 22 & 1;
        auto j1 = !(i1 ^ sign);
        auto j2 = !(i2 ^ sign);
        first = first & 0xf800 | sign << 10 | encoded >> 12 & 0x03ff;
        second = second & 0xd000 | j1 << 13 | j2 << 11 | encoded >> 1 & 0x07ff;
        if(!put16(output, location, first) || !put16(output, location + 2, second)) {
          return fail("Thumb branch relocation write failed");
        }
        continue;
      }
      return fail("unsupported ARM relocation type");
    }
  }

  auto copyArray = [&](u32 type, std::vector<u32>& destination) -> bool {
    for(u32 index = 0; index < object.sections.size(); index++) if(object.sections[index].type == type) {
      auto& placement = candidate.placements[index];
      if(!placement.allocated || object.sections[index].size % 4) return false;
      for(auto& relocation : object.relocations[index]) {
        if(relocation.type != R_ARM_ABS32 && relocation.type != R_ARM_TARGET1) return false;
      }
      auto& output = segment(placement.segment);
      for(u32 offset = 0; offset < object.sections[index].size; offset += 4) {
        u32 value = 0;
        if(!get32(output, placement.offset + offset, value)) return false;
        destination.push_back(value);
      }
    }
    return true;
  };
  if(!copyArray(SHT_PREINIT_ARRAY, candidate.preinit) || !copyArray(SHT_INIT_ARRAY, candidate.init)) {
    return fail("invalid initialization array");
  }
  linked = std::move(candidate);
  return true;
}
