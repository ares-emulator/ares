auto V30MZ::instructionDecimalAdjust(bool negate) -> void {
  wait(10 + negate);
  n8 al = AL;
  PSW.V = 0; // undefined
  if(PSW.AC || ((al & 0x0f) > 0x09)) {
    AL += negate ? -0x06 : 0x06;
    PSW.AC = 1;
    if(negate) PSW.V |= (al ^ 0x06) & (al ^ AL) & 0x80; // undefined
    else PSW.V |= (AL ^ al) & (AL ^ 0x06) & 0x80; // undefined
  } else {
    PSW.AC = 0;
  }
  if(PSW.CY || (al > 0x99)) {
    al = AL;
    AL += negate ? -0x60 : 0x60;
    PSW.CY = 1;
    if(negate) PSW.V |= (al ^ 0x60) & (al ^ AL) & 0x80; // undefined
    else PSW.V |= (AL ^ al) & (AL ^ 0x60) & 0x80; // undefined
  } else {
    PSW.CY = 0;
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
    PSW.S = 0; // undefined
    PSW.Z = 1; // undefined
  } else {
    AL &= 0x0f;
    PSW.AC = 0;
    PSW.CY = 0;
    PSW.S = 1; // undefined
    PSW.Z = 0; // undefined
  }
  AL &= 0x0f;
  PSW.V = 0; // undefined
  PSW.P = 1; // undefined
}

auto V30MZ::instructionAdjustAfterMultiply() -> void {
  wait(16);
  auto imm = fetch<Byte>();
  if(imm == 0) { interrupt(0, InterruptSource::CPU); return; }
  AH = AL / imm;
  AL %= imm;
  PSW.P = parity(AL);
  PSW.S = AW & 0x8000;
  PSW.Z = AW == 0;
}

auto V30MZ::instructionAdjustAfterDivide() -> void {
  wait(6);
  auto imm = fetch<Byte>();
  AL = ADD<Byte>(AL, MULU<Byte>(AH, imm) & 0xFF, 0);
  AH = 0;
}
