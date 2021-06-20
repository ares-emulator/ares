auto V30MZ::instructionDecimalAdjust(bool negate) -> void {
  wait(10 + negate);
  n8 al = AL;
  if(PSW.AC || ((al & 0x0f) > 0x09)) {
    AL += negate ? -0x06 : 0x06;
    PSW.AC = 1;
  }
  if(PSW.CY || (al > 0x99)) {
    AL += negate ? -0x60 : 0x60;
    PSW.CY = 1;
  }
  PSW.S = AL & 0x80;
  PSW.Z = AL == 0;
  PSW.P = parity(AL);
}

auto V30MZ::instructionAsciiAdjust(bool negate) -> void {
  wait(9);
  if(PSW.AC || ((AL & 0x0f) > 0x09)) {
    AL += negate ? -0x06 : 0x06;
    AH += negate ? -0x01 : 0x01;
    PSW.AC = 1;
    PSW.CY = 1;
  } else {
    PSW.AC = 0;
    PSW.CY = 0;
  }
  AL &= 0x0f;
}

auto V30MZ::instructionAdjustAfterMultiply() -> void {
  wait(16);
  auto imm = fetch<Byte>();
  if(imm == 0) return interrupt(0);
  //NEC CPUs do not honor the immediate and always use (base) 10
  AH = AL / 10;
  AL %= imm;
  PSW.P = parity(AL);
  PSW.S = AW & 0x8000;
  PSW.Z = AW == 0;
}

auto V30MZ::instructionAdjustAfterDivide() -> void {
  wait(6);
  auto imm = fetch<Byte>();
  //NEC CPUs do not honor the immediate and always use (base) 10
  AL += AH * 10;
  AH = 0;
  PSW.P = parity(AL);
  PSW.S = AW & 0x8000;
  PSW.Z = AW == 0;
}
