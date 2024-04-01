auto MOS6502::opcode() -> n8 {
  return read(PC++);
}

auto MOS6502::load(n8 addr) -> n8 {
  return read(addr);
}

auto MOS6502::push(n8 data) -> void {
  write(0x0100 | S--, data);
}

auto MOS6502::pull() -> n8 {
  return read(0x0100 | ++S);
}
