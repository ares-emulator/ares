inline auto HuC6280::load8(n8 address) -> n8 {
  step(CS);
  return read(MPR[1], address);
}

inline auto HuC6280::load16(n16 address) -> n8 {
  step(CS);
  return read(MPR[address >> 13], (n13)address);
}

inline auto HuC6280::store8(n8 address, n8 data) -> void {
  step(CS);
  return write(MPR[1], address, data);
}

inline auto HuC6280::store16(n16 address, n8 data) -> void {
  step(CS);
  return write(MPR[address >> 13], (n13)address, data);
}

//

auto HuC6280::idle() -> void {
  step(CS);
}

inline auto HuC6280::opcode() -> n8 {
  return load16(PC++);
}

inline auto HuC6280::operand() -> n8 {
  return load16(PC++);
}

//

inline auto HuC6280::push(n8 data) -> void {
  step(CS);
  write(MPR[1], 0x0100 | S--, data);
}

inline auto HuC6280::pull() -> n8 {
  step(CS);
  return read(MPR[1], 0x0100 | ++S);
}
