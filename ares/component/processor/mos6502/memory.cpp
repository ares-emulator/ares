auto MOS6502::idle() -> void {
  read(PC);
}

auto MOS6502::idlePageCrossed(n16 x, n16 y) -> void {
  if(x >> 8 == y >> 8) return;
  read((x & 0xff00) | (y & 0x00ff));
}

auto MOS6502::idlePageAlways(n16 x, n16 y) -> void {
  read((x & 0xff00) | (y & 0x00ff));
}

auto MOS6502::opcode() -> n8 {
  return read(PC++);
}

auto MOS6502::operand() -> n8 {
  return read(PC++);
}

auto MOS6502::load(n8 addr) -> n8 {
  return read(addr);
}

auto MOS6502::store(n8 addr, n8 data) -> void {
  write(addr, data);
}

auto MOS6502::push(n8 data) -> void {
  write(0x0100 | S--, data);
}

auto MOS6502::pull() -> n8 {
  return read(0x0100 | ++S);
}
