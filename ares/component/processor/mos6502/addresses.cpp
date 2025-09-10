auto MOS6502::addressAbsolute() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  return absolute;
}

auto MOS6502::addressAbsoluteJMP() -> n16 {
  n16 absolute = operand();
  // JMP Indirect last cycle
L absolute |= operand() << 8;
  return absolute;
}

auto MOS6502::addressAbsoluteXRead() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  idlePageCrossed(absolute, absolute + X);
  return absolute + X;
}

auto MOS6502::addressAbsoluteXWrite() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  idlePageAlways(absolute, absolute + X);
  return absolute + X;
}

auto MOS6502::addressAbsoluteYRead() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  idlePageCrossed(absolute, absolute + Y);
  return absolute + Y;
}

auto MOS6502::addressAbsoluteYWrite() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  idlePageAlways(absolute, absolute + Y);
  return absolute + Y;
}

auto MOS6502::addressImmediate() -> n16 {
  return PC++;
}

auto MOS6502::addressImplied() -> n16 {
  return PC;
}

auto MOS6502::addressIndirect() -> n16 {
  n16 absolute = operand();
  absolute |= operand() << 8;
  n16 indirect = read(absolute);
  absolute.byte(0)++;
  // JMP Indirect last cycle
L indirect |= read(absolute) << 8;
  return indirect;
}

auto MOS6502::addressIndirectX() -> n16 {
  n8 zeroPage = operand();
  idleZeroPage(zeroPage);
  n16 absolute = load(zeroPage + X);
  absolute |= load(zeroPage + X + 1) << 8;
  return absolute;
}

auto MOS6502::addressIndirectYRead() -> n16 {
  n16 zeroPage = operand();
  n16 absolute = load(zeroPage + 0);
  absolute |= load(zeroPage + 1) << 8;
  idlePageCrossed(absolute, absolute + Y);
  return absolute + Y;
}

auto MOS6502::addressIndirectYWrite() -> n16 {
  n8 zeroPage = operand();
  n16 absolute = load(zeroPage + 0);
  absolute |= load(zeroPage + 1) << 8;
  idlePageAlways(absolute, absolute + Y);
  return absolute + Y;
}

auto MOS6502::addressRelative() -> n16 {
  return PC++;
}

auto MOS6502::addressZeroPage() -> n16 {
  return operand();
}

auto MOS6502::addressZeroPageX() -> n16 {
  n8 zeropage = operand();
  idleZeroPage(zeropage);
  zeropage += X;
  return zeropage;
}

auto MOS6502::addressZeroPageY() -> n16 {
  n8 zeropage = operand();
  idleZeroPage(zeropage);
  zeropage += Y;
  return zeropage;
}
