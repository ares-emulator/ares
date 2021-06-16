auto V30MZ::instructionAddMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, ADD(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionAddRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, ADD(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionAddAccImm(Size size) -> void {
  wait(1);
  setAcc(size, ADD(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionOrMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, OR(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionOrRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, OR(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionOrAccImm(Size size) -> void {
  wait(1);
  setAcc(size, OR(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionAdcMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, ADC(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionAdcRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, ADC(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionAdcAccImm(Size size) -> void {
  wait(1);
  setAcc(size, ADC(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionSbbMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, SBB(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionSbbRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, SBB(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionSbbAccImm(Size size) -> void {
  wait(1);
  setAcc(size, SBB(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionAndMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, AND(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionAndRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, AND(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionAndAccImm(Size size) -> void {
  wait(1);
  setAcc(size, AND(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionSubMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, SUB(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionSubRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, SUB(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionSubAccImm(Size size) -> void {
  wait(1);
  setAcc(size, SUB(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionXorMemReg(Size size) -> void {
  wait(1);
  modRM();
  setMem(size, XOR(size, getMem(size), getReg(size)));
}

auto V30MZ::instructionXorRegMem(Size size) -> void {
  wait(1);
  modRM();
  setReg(size, XOR(size, getReg(size), getMem(size)));
}

auto V30MZ::instructionXorAccImm(Size size) -> void {
  wait(1);
  setAcc(size, XOR(size, getAcc(size), fetch(size)));
}

auto V30MZ::instructionCmpMemReg(Size size) -> void {
  wait(1);
  modRM();
  SUB(size, getMem(size), getReg(size));
}

auto V30MZ::instructionCmpRegMem(Size size) -> void {
  wait(1);
  modRM();
  SUB(size, getReg(size), getMem(size));
}

auto V30MZ::instructionCmpAccImm(Size size) -> void {
  wait(1);
  SUB(size, getAcc(size), fetch(size));
}

auto V30MZ::instructionTestAcc(Size size) -> void {
  wait(1);
  AND(size, getAcc(size), fetch(size));
}

auto V30MZ::instructionTestMemReg(Size size) -> void {
  wait(1);
  modRM();
  AND(size, getMem(size), getReg(size));
}

auto V30MZ::instructionMultiplySignedRegMemImm(Size size) -> void {
  wait(5);
  modRM();
  setReg(Word, MULI(Word, getMem(Word), size == Word ? (s16)fetch(Word) : (s8)fetch(Byte)));
}

auto V30MZ::instructionIncReg(u16& reg) -> void {
  wait(1);
  reg = INC(Word, reg);
}

auto V30MZ::instructionDecReg(u16& reg) -> void {
  wait(1);
  reg = DEC(Word, reg);
}

auto V30MZ::instructionSignExtendByte() -> void {
  wait(1);
  setAcc(Word, (i8)getAcc(Byte));
}

auto V30MZ::instructionSignExtendWord() -> void {
  wait(1);
  setAcc(Long, (i16)getAcc(Word));
}
