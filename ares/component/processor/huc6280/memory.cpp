auto HuC6280::load8(uint8 address) -> uint8 {
  step(CS);
  return read(MPR[1], address);
}

auto HuC6280::load16(uint16 address) -> uint8 {
  step(CS);
  return read(MPR[address >> 13], (uint13)address);
}

auto HuC6280::store8(uint8 address, uint8 data) -> void {
  step(CS);
  return write(MPR[1], address, data);
}

auto HuC6280::store16(uint16 address, uint8 data) -> void {
  step(CS);
  return write(MPR[address >> 13], (uint13)address, data);
}

//

auto HuC6280::idle() -> void {
  step(CS);
}

auto HuC6280::opcode() -> uint8 {
  return load16(PC++);
}

auto HuC6280::operand() -> uint8 {
  return load16(PC++);
}

//

auto HuC6280::push(uint8 data) -> void {
  step(CS);
  write(MPR[1], 0x0100 | S--, data);
}

auto HuC6280::pull() -> uint8 {
  step(CS);
  return read(MPR[1], 0x0100 | ++S);
}
