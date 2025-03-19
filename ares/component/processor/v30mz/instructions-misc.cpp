auto V30MZ::instructionSegment(n16 segment) -> void {
  prefix.segment = opcode;
  prefix.count++;
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionRepeat() -> void {
  prefix.repeat = opcode;
  prefix.count++;
  wait(4);
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionLock() -> void {
  prefix.lock = opcode;
  prefix.count++;
  state.prefix = 1;
  state.poll = 0;
}

auto V30MZ::instructionWait() -> void {
  wait(10);
}

auto V30MZ::instructionHalt() -> void {
  wait(9);
  state.halt = 1;
}

auto V30MZ::instructionNop() -> void {
  wait(1);
}

auto V30MZ::instructionUndefined() -> void {
  wait(1);
}

auto V30MZ::instructionUndefined1() -> void {
  wait(1);
  fetch<Byte>();
}

template<u32 size> auto V30MZ::instructionIn() -> void {
  // TODO: The exact cycle on which I/O access is performed remains unknown.
  wait(5);
  setAccumulator<size>(in<size>(fetch<Byte>()));
  wait(1);
}

template<u32 size> auto V30MZ::instructionOut() -> void {
  // TODO: The exact cycle on which I/O access is performed remains unknown.
  wait(6);
  out<size>(fetch<Byte>(), getAccumulator<size>());
}

template<u32 size> auto V30MZ::instructionInDW() -> void {
  // TODO: The exact cycle on which I/O access is performed remains unknown.
  wait(4);
  setAccumulator<size>(in<size>(DW));
  wait(1);
}

template<u32 size> auto V30MZ::instructionOutDW() -> void {
  // TODO: The exact cycle on which I/O access is performed remains unknown.
  wait(5);
  out<size>(DW, getAccumulator<size>());
}

auto V30MZ::instructionSetALCarry() -> void {
  wait(8);
  AL = PSW.CY ? 0xFF : 0x00;
}

auto V30MZ::instructionTranslate() -> void {
  wait(4);
  AL = read<Byte>(segment(DS0), BW + AL);
}

auto V30MZ::instructionBound() -> void {
  wait(12);
  modRM();
  i16 lo = (i16)getMemory<Word>(0);
  i16 hi = (i16)getMemory<Word>(2);
  i16 reg = (i16)getRegister<Word>();
  if(reg < lo || reg > hi) interrupt(5, InterruptSource::CPU);
}
