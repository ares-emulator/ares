auto CPU::DataCache::Line::hit(u32 paddr) const -> bool {
  return valid && tag == (paddr & ~0x0000'0fff);
}

auto CPU::DataCache::Line::fill(u32 paddr) -> void {
  cpu.step(40 * 2);
  valid  = 1;
  dirty  = 0;
  tag    = paddr & ~0x0000'0fff;
  fillPc = cpu.ipu.pc;
  cpu.busReadBurst<DCache>(tag | index, words);
}

auto CPU::DataCache::Line::writeBack() -> void {
  cpu.step(40 * 2);
  dirty = 0;
  cpu.busWriteBurst<DCache>(tag | index, words);
}

auto CPU::DataCache::line(u64 vaddr) -> Line& {
  return lines[vaddr >> 4 & 0x1ff];
}

template<u32 Size>
auto CPU::DataCache::Line::read(u32 paddr) const -> u64 {
  if constexpr(Size == Byte) { return bytes[paddr >> 0 & 15 ^ 3]; }
  if constexpr(Size == Half) { return halfs[paddr >> 1 &  7 ^ 1]; }
  if constexpr(Size == Word) { return words[paddr >> 2 &  3 ^ 0]; }
  if constexpr(Size == Dual) {
    u64 upper = words[paddr >> 2 & 2 | 0];
    u64 lower = words[paddr >> 2 & 2 | 1];
    return upper << 32 | lower << 0;
  }
}

template<u32 Size>
auto CPU::DataCache::Line::write(u32 paddr, u64 data) -> void {
  if constexpr(Size == Byte) { bytes[paddr >> 0 & 15 ^ 3] = data; }
  if constexpr(Size == Half) { halfs[paddr >> 1 &  7 ^ 1] = data; }
  if constexpr(Size == Word) { words[paddr >> 2 &  3 ^ 0] = data; }
  if constexpr(Size == Dual) {
    words[paddr >> 2 & 2 | 0] = data >> 32;
    words[paddr >> 2 & 2 | 1] = data >>  0;
  }
  dirty |= ((1 << Size) - 1) << (paddr & 0xF);
  dirtyPc = cpu.ipu.pc;
}

template<u32 Size>
auto CPU::DataCache::read(u64 vaddr, u32 paddr) -> u64 {
  auto& line = this->line(vaddr);
  if(!line.hit(paddr)) {
    if(line.valid && line.dirty) line.writeBack();
    line.fill(paddr);
  } else {
    cpu.step(1 * 2);
  }
  return line.read<Size>(paddr);
}

template<u32 Size>
auto CPU::DataCache::readDebug(u64 vaddr, u32 paddr) -> u64 {
  // This is used by the debugger to read memory through the cache but without
  // actually causing side effects that modify the cache state (eg: no line fill)
  auto& line = this->line(vaddr);
  if(!line.hit(paddr)) {
    Thread dummyThread{};
    return bus.read<Size>(paddr, dummyThread, "Ares Debugger");
  }
  return line.read<Size>(paddr);
}

template<u32 Size>
auto CPU::DataCache::write(u64 vaddr, u32 paddr, u64 data) -> void {
  auto& line = this->line(vaddr);
  if(!line.hit(paddr)) {
    if(line.valid && line.dirty) line.writeBack();
    line.fill(paddr);
  } else {
    cpu.step(1 * 2);
  }
  line.write<Size>(paddr, data);
}

template<u32 Size>
auto CPU::DataCache::writeDebug(u64 vaddr, u32 paddr, u64 data) -> void {
  // This is used by the debugger to write memory through the cache but without
  // actually causing side effects that modify the cache state (eg: no line fill)
  auto& line = this->line(vaddr);
  if(!line.hit(paddr)) {
    Thread dummyThread{};
    return bus.write<Size>(paddr, data, dummyThread, "Ares Debugger");
  }
  line.write<Size>(paddr, data);
}

auto CPU::DataCache::power(bool reset) -> void {
  u32 index = 0;
  for(auto& line : lines) {
    line.valid = 0;
    line.dirty = 0;
    line.tag   = 0;
    line.index = index++ << 4 & 0xff0;
    for(auto& word : line.words) word = 0;
  }
}

template auto CPU::DataCache::Line::write<Byte>(u32 paddr, u64 data) -> void;
template auto CPU::DataCache::writeDebug<Byte>(u64 vaddr, u32 paddr, u64 data) -> void;
template auto CPU::DataCache::readDebug<Byte>(u64 vaddr, u32 paddr) -> u64;
