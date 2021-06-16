auto V30MZ::instructionGroup1MemImm(Size size, bool sign) -> void {
  wait(1);
  modRM();
  auto mem = getMem(size);
  n16 imm = 0;
  if(sign) imm = (i8)fetch();
  else if(size == Byte) imm = fetch();
  else imm = fetch(Word);
  switch(modrm.reg) {
  case 0: setMem(size, ADD(size, mem, imm)); break;
  case 1: setMem(size, OR (size, mem, imm)); break;
  case 2: setMem(size, ADC(size, mem, imm)); break;
  case 3: setMem(size, SBB(size, mem, imm)); break;
  case 4: setMem(size, AND(size, mem, imm)); break;
  case 5: setMem(size, SUB(size, mem, imm)); break;
  case 6: setMem(size, XOR(size, mem, imm)); break;
  case 7:              SUB(size, mem, imm);  break;
  }
}

auto V30MZ::instructionGroup2MemImm(Size size, u8 clocks, maybe<n8> imm) -> void {
  wait(clocks);
  modRM();
  auto mem = getMem(size);
  if(!imm) imm = fetch();
  switch(modrm.reg) {
  case 0: setMem(size, ROL(size, mem, *imm)); break;
  case 1: setMem(size, ROR(size, mem, *imm)); break;
  case 2: setMem(size, RCL(size, mem, *imm)); break;
  case 3: setMem(size, RCR(size, mem, *imm)); break;
  case 4: setMem(size, SHL(size, mem, *imm)); break;
  case 5: setMem(size, SHR(size, mem, *imm)); break;
  case 6: setMem(size, SAL(size, mem, *imm)); break;
  case 7: setMem(size, SAR(size, mem, *imm)); break;
  }
}

auto V30MZ::instructionGroup3MemImm(Size size) -> void {
  modRM();
  auto mem = getMem(size);
  switch(modrm.reg) {
  case 0: wait(1); AND(size, mem, fetch(size)); break;  //TEST
  case 1: wait(1); AND(size, mem, fetch(size)); break;  //TEST (undocumented mirror)
  case 2: wait(1); setMem(size, NOT(size, mem)); break;
  case 3: wait(1); setMem(size, NEG(size, mem)); break;
  case 4: wait(3); setAcc(size * 2, MULU(size, getAcc(size), mem)); break;
  case 5: wait(3); setAcc(size * 2, MULI(size, getAcc(size), mem)); break; break;
  case 6: wait(size == Byte ? 15 : 23); setAcc(size * 2, DIVU(size, getAcc(size * 2), mem)); break;
  case 7: wait(size == Byte ? 17 : 24); setAcc(size * 2, DIVI(size, getAcc(size * 2), mem)); break;
  }
}

auto V30MZ::instructionGroup4MemImm(Size size) -> void {
  modRM();
  switch(modrm.reg) {
  case 0:  //INC
    wait(1);
    setMem(size, INC(size, getMem(size)));
    break;
  case 1:  //DEC
    wait(1);
    setMem(size, DEC(size, getMem(size)));
    break;
  case 2:  //CALL
    wait(5);
    push(r.ip);
    r.ip = getMem(Word);
    flush();
    break;
  case 3:  //CALLF
    wait(11);
    push(r.cs);
    push(r.ip);
    r.ip = getMem(Word, 0);
    r.cs = getMem(Word, 2);
    flush();
    break;
  case 4:  //JMP
    wait(4);
    r.ip = getMem(Word);
    flush();
    break;
  case 5:  //JMPF
    wait(9);
    r.ip = getMem(Word, 0);
    r.cs = getMem(Word, 2);
    flush();
    break;
  case 6:  //PUSH
    push(getMem(Word));
    break;
  case 7:  //PUSH (undocumented mirror)
    push(getMem(Word));
    break;
  }
}
