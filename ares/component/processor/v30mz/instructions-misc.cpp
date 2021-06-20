auto V30MZ::instructionSegment(n16 segment) -> void {
  if(prefixes.full()) prefixes.read(0);
  prefixes.write(opcode);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionRepeat() -> void {
  if(prefixes.full()) prefixes.read(0);
  prefixes.write(opcode);
  wait(4);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionLock() -> void {
  if(prefixes.full()) prefixes.read(0);
  prefixes.write(opcode);
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

template<u32 size> auto V30MZ::instructionIn() -> void {
  wait(6);
  setAccumulator<size>(in<size>(fetch<Byte>()));
}

template<u32 size> auto V30MZ::instructionOut() -> void {
  wait(6);
  out<size>(fetch<Byte>(), getAccumulator<size>());
}

template<u32 size> auto V30MZ::instructionInDW() -> void {
  wait(4);
  setAccumulator<size>(in<size>(DW));
}

template<u32 size> auto V30MZ::instructionOutDW() -> void {
  wait(4);
  out<size>(DW, getAccumulator<size>());
}

auto V30MZ::instructionTranslate(u8 clocks) -> void {
  wait(clocks);
  AL = read<Byte>(segment(DS0), BW + AL);
}

auto V30MZ::instructionBound() -> void {
  wait(12);
  modRM();
  auto lo = getMemory<Word>(0);
  auto hi = getMemory<Word>(2);
  auto reg = getRegister<Word>();
  if(reg < lo || reg > hi) interrupt(5);
}
