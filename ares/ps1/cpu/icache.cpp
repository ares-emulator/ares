alwaysinline auto CPU::InstructionCache::fetch(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  u32 word = address >> 2 & 3;
  if(likely(line.tag == (address & TagMask) && (line.valid & (1u << word)))) {
    cpu.step(1);
    return line.words[word];
  }
  return fetchSlow(address);
}
auto CPU::InstructionCache::fetchSlow(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  u32 word = address >> 2 & 3;
  u32 tag = address & TagMask;
  bool tagMatch = line.tag == tag;
  if(unlikely(!tagMatch || !(line.valid & 1 << word))) {
    if(!tagMatch) line.valid = 0;
    u32 start = address & ~3u;
    u32 size = Word * (4 - word);
    if((address & 0x1fff'ffff) <= 0x007f'ffff) {
      cpu.beginBusTransaction(start, size);
      u32 base = (address & ~0xfu) & cpu.ram.maskWord;
      auto* src = (const u32*)&cpu.ram.data[base];
      for(u32 index = word; index < 4; index++) line.words[index] = src[index];
      cpu.endBusTransaction();
    }
    if((address & 0x1fff'ffff) >= 0x1fc0'0000) {
      cpu.beginBusTransaction(start, size);
      for(u32 index = word; index < 4; index++) {
        line.words[index] = bios.read<Word>((address & ~0xfu) | index << 2);
      }
      cpu.endBusTransaction();
    }
    if((address & 0x1fff'ffff) > 0x007f'ffff && (address & 0x1fff'ffff) < 0x1fc0'0000) {
      cpu.beginBusTransaction(start, size);
      for(u32 index = word; index < 4; index++) {
        line.words[index] = bus.read<Word>(((address & ~0xfu) | index << 2) & 0x1fff'ffff);
      }
      cpu.endBusTransaction();
    }
    for(u32 index = word; index < 4; index++) line.valid |= 1 << index;
    line.tag = tag | (line.tag & 2);
  } else {
    cpu.step(1);
  }
  return line.words[address >> 2 & 3];
}

inline auto CPU::InstructionCache::read(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  return line.words[address >> 2 & 3];
}

inline auto CPU::InstructionCache::writeTag(u32 address, u32 data) -> void {
  auto& line = lines[address >> 4 & 0xff];
  line.tag = (address & TagMask) | (line.tag & 2);
  line.valid = data & 0x0f;
}

inline auto CPU::InstructionCache::writeData(u32 address, u32 data) -> void {
  auto& line = lines[address >> 4 & 0xff];
  line.words[address >> 2 & 3] = data;
}

auto CPU::InstructionCache::enable(bool enable) -> void {
  if(!enable) {
    for(auto& line : lines) line.tag |=  2;
  } else {
    for(auto& line : lines) line.tag &= ~2;
  }
}

auto CPU::InstructionCache::power(bool reset) -> void {
  for(auto& line : lines) {
    line.tag = 1;
    line.valid = 0;
    for(auto& word : line.words) word = random();
  }
}
