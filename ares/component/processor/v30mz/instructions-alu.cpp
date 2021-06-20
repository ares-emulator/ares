template<u32 size> auto V30MZ::instructionAddMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(ADD<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionAddRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(ADD<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionAddAccImm() -> void {
  wait(1);
  setAccumulator<size>(ADD<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionOrMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(OR<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionOrRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(OR<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionOrAccImm() -> void {
  wait(1);
  setAccumulator<size>(OR<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionAdcMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(ADC<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionAdcRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(ADC<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionAdcAccImm() -> void {
  wait(1);
  setAccumulator<size>(ADC<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionSbbMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(SBB<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionSbbRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(SBB<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionSbbAccImm() -> void {
  wait(1);
  setAccumulator<size>(SBB<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionAndMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(AND<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionAndRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(AND<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionAndAccImm() -> void {
  wait(1);
  setAccumulator<size>(AND<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionSubMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(SUB<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionSubRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(SUB<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionSubAccImm() -> void {
  wait(1);
  setAccumulator<size>(SUB<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionXorMemReg() -> void {
  wait(1);
  modRM();
  setMemory<size>(XOR<size>(getMemory<size>(), getRegister<size>()));
}

template<u32 size> auto V30MZ::instructionXorRegMem() -> void {
  wait(1);
  modRM();
  setRegister<size>(XOR<size>(getRegister<size>(), getMemory<size>()));
}

template<u32 size> auto V30MZ::instructionXorAccImm() -> void {
  wait(1);
  setAccumulator<size>(XOR<size>(getAccumulator<size>(), fetch<size>()));
}

template<u32 size> auto V30MZ::instructionCmpMemReg() -> void {
  wait(1);
  modRM();
  SUB<size>(getMemory<size>(), getRegister<size>());
}

template<u32 size> auto V30MZ::instructionCmpRegMem() -> void {
  wait(1);
  modRM();
  SUB<size>(getRegister<size>(), getMemory<size>());
}

template<u32 size> auto V30MZ::instructionCmpAccImm() -> void {
  wait(1);
  SUB<size>(getAccumulator<size>(), fetch<size>());
}

template<u32 size> auto V30MZ::instructionTestAcc() -> void {
  wait(1);
  AND<size>(getAccumulator<size>(), fetch<size>());
}

template<u32 size> auto V30MZ::instructionTestMemReg() -> void {
  wait(1);
  modRM();
  AND<size>(getMemory<size>(), getRegister<size>());
}

template<u32 size> auto V30MZ::instructionMultiplySignedRegMemImm() -> void {
  wait(5);
  modRM();
  setRegister<Word>(MULI<Word>(getMemory<Word>(), size == Word ? (s16)fetch<Word>() : (s8)fetch<Byte>()));
}

auto V30MZ::instructionIncReg(u16& reg) -> void {
  wait(1);
  reg = INC<Word>(reg);
}

auto V30MZ::instructionDecReg(u16& reg) -> void {
  wait(1);
  reg = DEC<Word>(reg);
}

auto V30MZ::instructionSignExtendByte() -> void {
  wait(1);
  setAccumulator<Word>((i8)getAccumulator<Byte>());
}

auto V30MZ::instructionSignExtendWord() -> void {
  wait(1);
  setAccumulator<Long>((i16)getAccumulator<Word>());
}
