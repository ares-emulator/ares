auto CPU::InstructionCache::Line::hit(u32 address) const -> bool {
  return tag == (address & ~0xfff);
}

auto CPU::InstructionCache::Line::fill() -> void {
//  cpu.step(48);
  words[0] = bus.read<Word>(tag | index | 0x00);
  words[1] = bus.read<Word>(tag | index | 0x04);
  words[2] = bus.read<Word>(tag | index | 0x08);
  words[3] = bus.read<Word>(tag | index | 0x0c);
  words[4] = bus.read<Word>(tag | index | 0x10);
  words[5] = bus.read<Word>(tag | index | 0x14);
  words[6] = bus.read<Word>(tag | index | 0x18);
  words[7] = bus.read<Word>(tag | index | 0x1c);
}

auto CPU::InstructionCache::Line::writeBack() -> void {
//  cpu.step(48);
  bus.write<Word>(tag | index | 0x00, words[0]);
  bus.write<Word>(tag | index | 0x04, words[1]);
  bus.write<Word>(tag | index | 0x08, words[2]);
  bus.write<Word>(tag | index | 0x0c, words[3]);
  bus.write<Word>(tag | index | 0x10, words[4]);
  bus.write<Word>(tag | index | 0x14, words[5]);
  bus.write<Word>(tag | index | 0x18, words[6]);
  bus.write<Word>(tag | index | 0x1c, words[7]);
}

auto CPU::InstructionCache::Line::read(u32 address) const -> u32 {
  return words[address >> 2 & 7];
}

auto CPU::InstructionCache::line(u32 address) -> Line& {
  return lines[address >> 5 & 0x1ff];
}

//used by the recompiler to simulate instruction cache fetch timing
auto CPU::InstructionCache::step(u32 address) -> void {
  auto& line = this->line(address);
  if(!line.hit(address) || !line.valid) {
    cpu.step(48);
    line.tag = address & ~0xfff;
    line.valid = 1;
  }
  cpu.step(1);
}

//used by the interpreter to fully emulate the instruction cache
auto CPU::InstructionCache::fetch(u32 address) -> u32 {
  auto& line = this->line(address);
  if(!line.hit(address) || !line.valid) {
    line.tag = address & ~0xfff;
    line.fill();
    line.valid = 1;
  }
  cpu.step(1);
  return line.read(address);
}

auto CPU::InstructionCache::power(bool reset) -> void {
  u32 index = 0;
  for(auto& line : lines) {
    line.index = index++ << 5;
    for(auto& word : line.words) word = 0;
    line.tag = 0;
    line.valid = 0;
  }
}
