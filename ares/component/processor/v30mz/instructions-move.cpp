template<u32 size> auto V30MZ::instructionMoveMemReg() -> void {
  modRM();
  if(modrm.mod == 3) wait(1);
  setMemory<size>(getRegister<size>());
}

template<u32 size> auto V30MZ::instructionMoveRegMem() -> void {
  modRM();
  if(modrm.mod == 3) wait(1);
  setRegister<size>(getMemory<size>());
}

auto V30MZ::instructionMoveMemSeg() -> void {
  wait(1);
  modRM();
  setMemory<Word>(getSegment());
  state.poll = 0;
}

auto V30MZ::instructionMoveSegMem() -> void {
  wait(2);
  modRM();
  setSegment(getMemory<Word>());
  if((modrm.reg & 3) == 3) state.poll = 0;
}

template<u32 size> auto V30MZ::instructionMoveAccMem() -> void {
  setAccumulator<size>(read<size>(segment(DS0), fetch<Word>()));
}

template<u32 size> auto V30MZ::instructionMoveMemAcc() -> void {
  write<size>(segment(DS0), fetch<Word>(), getAccumulator<size>());
}

auto V30MZ::instructionMoveRegImm(u8& reg) -> void {
  wait(1);
  reg = fetch<Byte>();
}

auto V30MZ::instructionMoveRegImm(u16& reg) -> void {
  wait(1);
  reg = fetch<Word>();
}

template<u32 size> auto V30MZ::instructionMoveMemImm() -> void {
  modRM();
  setMemory<size>(fetch<size>());
}

auto V30MZ::instructionExchange(u16& x, u16& y) -> void {
  wait(3);
  n16 z = x;
  x = y;
  y = z;
}

template<u32 size> auto V30MZ::instructionExchangeMemReg() -> void {
  wait(3);
  modRM();
  auto mem = getMemory<size>();
  auto reg = getRegister<size>();
  setMemory<size>(reg);
  setRegister<size>(mem);
}

auto V30MZ::instructionLoadEffectiveAddressRegMem() -> void {
  wait(1);
  modRM();
  setRegister<Word>(modrm.address);
}

auto V30MZ::instructionLoadSegmentMem(u16& segment) -> void {
  wait(4);
  modRM();
  setRegister<Word>(getMemory<Word>());
  segment = getMemory<Word>(2);
}
