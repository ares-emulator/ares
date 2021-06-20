auto V30MZ::instructionStoreFlagsAcc() -> void {
  wait(4);
  PSW = (PSW & 0xff00) | AH;
}

auto V30MZ::instructionLoadAccFlags() -> void {
  wait(2);
  AH = (PSW & 0x00ff);
}

auto V30MZ::instructionComplementCarry() -> void {
  wait(4);
  PSW.CY = !PSW.CY;
}

auto V30MZ::instructionClearFlag(u32 bit) -> void {
  wait(4);
  PSW &= ~(1 << bit);
}

auto V30MZ::instructionSetFlag(u32 bit) -> void {
  wait(4);
  PSW |= 1 << bit;
  if(bit == PSW.IE.bit()) state.poll = 0;
}
