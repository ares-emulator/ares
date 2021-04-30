//used by the recompiler to simulate instruction cache fetch timing
auto CPU::InstructionCache::step(u32 address) -> void {
  auto& line = lines[address >> 4 & 0xff];
  if(line.tag != (address & 0x1fff'fff0)) {
    if((address & 0x1fff'ffff) <= 0x007f'ffff) cpu.step(4 *  4);
    if((address & 0x1fff'ffff) >= 0x1fc0'0000) cpu.step(4 * 24);
    line.tag = address & 0x1fff'fff0 | line.tag & 0x0000'000e;
  } else {
    cpu.step(1);
  }
}

//used by the interpreter to fully emulate the instruction cache
inline auto CPU::InstructionCache::fetch(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  if(line.tag != (address & 0x1fff'fff0)) {
    //reload line
    if((address & 0x1fff'ffff) <= 0x007f'ffff) {
      line.words[0] = cpu.ram.read<Word>(address & ~0xf | 0x0);
      line.words[1] = cpu.ram.read<Word>(address & ~0xf | 0x4);
      line.words[2] = cpu.ram.read<Word>(address & ~0xf | 0x8);
      line.words[3] = cpu.ram.read<Word>(address & ~0xf | 0xc);
      cpu.step(4 * 4);
    }
    if((address & 0x1fff'ffff) >= 0x1fc0'0000) {
      line.words[0] = bios.read<Word>(address & ~0xf | 0x0);
      line.words[1] = bios.read<Word>(address & ~0xf | 0x4);
      line.words[2] = bios.read<Word>(address & ~0xf | 0x8);
      line.words[3] = bios.read<Word>(address & ~0xf | 0xc);
      cpu.step(4 * 24);
    }
    //update address and mark tag as valid
    line.tag = address & 0x1fff'fff0 | line.tag & 0x0000'000e;
  } else {
    cpu.step(1);
  }
  return line.words[address >> 2 & 3];
}

inline auto CPU::InstructionCache::read(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 0xff];
  return line.words[address >> 2 & 3];
}

inline auto CPU::InstructionCache::invalidate(u32 address) -> void {
  auto& line = lines[address >> 4 & 0xff];
  line.tag |= 1;  //mark tag as invalid
}

auto CPU::InstructionCache::enable(bool enable) -> void {
  if(!enable) {
    for(auto& line : lines) line.tag |=  2;  //mark tag as disabled
  } else {
    for(auto& line : lines) line.tag &= ~2;  //mark tag as enabled
  }
}

auto CPU::InstructionCache::power(bool reset) -> void {
  for(auto& line : lines) {
    line.tag = 1;  //mark tag as invalid
    for(auto& word : line.words) word = 0;  //should be random
  }
}
