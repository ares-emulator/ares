auto V30MZ::instructionSegment(n16 segment) -> void {
  if(prefixes.size() >= 7) prefixes.removeRight();
  prefixes.prepend(opcode);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionRepeat() -> void {
  if(prefixes.size() >= 7) prefixes.removeRight();
  prefixes.prepend(opcode);
  wait(4);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionLock() -> void {
  if(prefixes.size() >= 7) prefixes.removeRight();
  prefixes.prepend(opcode);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionWait() -> void {
  wait(1);
}

auto V30MZ::instructionHalt() -> void {
  wait(8);
  state.halt = 1;
}

auto V30MZ::instructionNop() -> void {
  wait(1);
}

auto V30MZ::instructionIn(Size size) -> void {
  wait(6);
  setAcc(size, in(size, fetch()));
}

auto V30MZ::instructionOut(Size size) -> void {
  wait(6);
  out(size, fetch(), getAcc(size));
}

auto V30MZ::instructionInDX(Size size) -> void {
  wait(4);
  setAcc(size, in(size, r.dx));
}

auto V30MZ::instructionOutDX(Size size) -> void {
  wait(4);
  out(size, r.dx, getAcc(size));
}

auto V30MZ::instructionTranslate(u8 clocks) -> void {
  wait(clocks);
  r.al = read(Byte, segment(r.ds), r.bx + r.al);
}

auto V30MZ::instructionBound() -> void {
  wait(12);
  modRM();
  auto lo = getMem(Word, 0);
  auto hi = getMem(Word, 2);
  auto reg = getReg(Word);
  if(reg < lo || reg > hi) interrupt(5);
}
