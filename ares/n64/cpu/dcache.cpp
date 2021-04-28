auto CPU::DataCache::Line::hit(u32 address) const -> bool {
  return tag == (address & ~0xfff);
}

template<u32 Size> auto CPU::DataCache::Line::fill(u32 address, u64 data) -> void {
//  cpu.step(40);
  //read words according to critical doubleword first scheme
  switch(address & 8) {
  case 0:
    if constexpr(Size != Dual) {
      words[0] = bus.read<Word>(tag | index | 0x0);
      words[1] = bus.read<Word>(tag | index | 0x4);
    }
    write<Size>(address, data);
    words[2] = bus.read<Word>(tag | index | 0x8);
    words[3] = bus.read<Word>(tag | index | 0xc);
    break;
  case 8:
    if constexpr(Size != Dual) {
      words[2] = bus.read<Word>(tag | index | 0x8);
      words[3] = bus.read<Word>(tag | index | 0xc);
    }
    write<Size>(address, data);
    words[0] = bus.read<Word>(tag | index | 0x0);
    words[1] = bus.read<Word>(tag | index | 0x4);
    break;
  }
}

auto CPU::DataCache::Line::fill(u32 address) -> void {
//  cpu.step(40);
  //read words according to critical doubleword first scheme
  switch(address & 8) {
  case 0:
    words[0] = bus.read<Word>(tag | index | 0x0);
    words[1] = bus.read<Word>(tag | index | 0x4);
    words[2] = bus.read<Word>(tag | index | 0x8);
    words[3] = bus.read<Word>(tag | index | 0xc);
    break;
  case 8:
    words[2] = bus.read<Word>(tag | index | 0x8);
    words[3] = bus.read<Word>(tag | index | 0xc);
    words[0] = bus.read<Word>(tag | index | 0x0);
    words[1] = bus.read<Word>(tag | index | 0x4);
    break;
  }
}

auto CPU::DataCache::Line::writeBack() -> void {
//  cpu.step(40);
  bus.write<Word>(tag | index | 0x0, words[0]);
  bus.write<Word>(tag | index | 0x4, words[1]);
  bus.write<Word>(tag | index | 0x8, words[2]);
  bus.write<Word>(tag | index | 0xc, words[3]);
}

auto CPU::DataCache::line(u32 address) -> Line& {
  return lines[address >> 4 & 0x1ff];
}

template<u32 Size>
auto CPU::DataCache::Line::read(u32 address) const -> u64 {
  if constexpr(Size == Byte) {
    auto data = words[address >> 2 & 3];
    return u8(data >> (address & 3 ^ 3) * 8);
  }
  if constexpr(Size == Half) {
    auto data = words[address >> 2 & 3];
    return u16(data >> (address & 2 ^ 2) * 8);
  }
  if constexpr(Size == Word) {
    return words[address >> 2 & 3];
  }
  if constexpr(Size == Dual) {
    auto hi = words[address >> 2 & 2 | 0];
    auto lo = words[address >> 2 & 2 | 1];
    return (u64)hi << 32 | (u64)lo << 0;
  }
}

template<u32 Size>
auto CPU::DataCache::Line::write(u32 address, u64 data) -> void {
  if constexpr(Size == Byte) {
    auto shift = (address & 3 ^ 3) * 8;
    words[address >> 2 & 3] &= ~(0xff << shift);
    words[address >> 2 & 3] |= u8(data) << shift;
  }
  if constexpr(Size == Half) {
    auto shift = (address & 2 ^ 2) * 8;
    words[address >> 2 & 3] &= ~(0xffff << shift);
    words[address >> 2 & 3] |= u16(data) << shift;
  }
  if constexpr(Size == Word) {
    words[address >> 2 & 3] = data;
  }
  if constexpr(Size == Dual) {
    words[address >> 2 & 2 | 0] = data >> 32;
    words[address >> 2 & 2 | 1] = data >>  0;
  }
}

template<u32 Size>
auto CPU::DataCache::read(u32 address) -> u64 {
  auto& line = this->line(address);
  if(!line.hit(address) || !line.valid) {
    if(line.dirty) line.writeBack();
    line.tag = address & ~0xfff;
    line.fill(address);
    line.valid = 1;
    line.dirty = 0;
  }
  cpu.step(1);
  return line.read<Size>(address);
}

template<u32 Size>
auto CPU::DataCache::write(u32 address, u64 data) -> void {
  auto& line = this->line(address);
  if(!line.hit(address) || !line.valid) {
    if(line.dirty) line.writeBack();
    cpu.step(1);
    line.tag = address & ~0xfff;
    line.fill<Size>(address, data);
    line.valid = 1;
    line.dirty = 1;
    return;
  }
  cpu.step(1);
  line.write<Size>(address, data);
  line.dirty = 1;
}

auto CPU::DataCache::power(bool reset) -> void {
  u32 index = 0;
  for(auto& line : lines) {
    line.index = index++ << 4;
    for(auto& word : line.words) word = 0;
    line.tag = 0;
    line.valid = 0;
    line.dirty = 0;
  }
}
