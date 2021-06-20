template<u32 size> auto V30MZ::instructionGroup1MemImm(bool sign) -> void {
  wait(1);
  modRM();
  auto mem = getMemory<size>();
  n16 imm = 0;
  if(sign) imm = (i8)fetch<Byte>();
  else if(size == Byte) imm = fetch<Byte>();
  else imm = fetch<Word>();
  switch(modrm.reg) {
  case 0: setMemory<size>(ADD<size>(mem, imm)); break;
  case 1: setMemory<size>(OR <size>(mem, imm)); break;
  case 2: setMemory<size>(ADC<size>(mem, imm)); break;
  case 3: setMemory<size>(SBB<size>(mem, imm)); break;
  case 4: setMemory<size>(AND<size>(mem, imm)); break;
  case 5: setMemory<size>(SUB<size>(mem, imm)); break;
  case 6: setMemory<size>(XOR<size>(mem, imm)); break;
  case 7:                (SUB<size>(mem, imm)); break;
  }
}

template<u32 size> auto V30MZ::instructionGroup2MemImm(u8 clocks, maybe<u8> imm) -> void {
  wait(clocks);
  modRM();
  auto mem = getMemory<size>();
  if(!imm) imm = fetch<Byte>();
  switch(modrm.reg) {
  case 0: setMemory<size>(ROL<size>(mem, *imm)); break;
  case 1: setMemory<size>(ROR<size>(mem, *imm)); break;
  case 2: setMemory<size>(RCL<size>(mem, *imm)); break;
  case 3: setMemory<size>(RCR<size>(mem, *imm)); break;
  case 4: setMemory<size>(SHL<size>(mem, *imm)); break;
  case 5: setMemory<size>(SHR<size>(mem, *imm)); break;
  case 6: setMemory<size>(SAL<size>(mem, *imm)); break;
  case 7: setMemory<size>(SAR<size>(mem, *imm)); break;
  }
}

template<u32 size> auto V30MZ::instructionGroup3MemImm() -> void {
  modRM();
  auto mem = getMemory<size>();
  switch(modrm.reg) {
  case 0: wait(1); AND<size>(mem, fetch<size>()); break;  //TEST
  case 1: wait(1); AND<size>(mem, fetch<size>()); break;  //TEST (undocumented mirror)
  case 2: wait(1); setMemory<size>(NOT<size>(mem)); break;
  case 3: wait(1); setMemory<size>(NEG<size>(mem)); break;
  case 4: wait(3); setAccumulator<size * 2>(MULU<size>(getAccumulator<size>(), mem)); break;
  case 5: wait(3); setAccumulator<size * 2>(MULI<size>(getAccumulator<size>(), mem)); break; break;
  case 6: wait(size == Byte ? 15 : 23); setAccumulator<size * 2>(DIVU<size>(getAccumulator<size * 2>(), mem)); break;
  case 7: wait(size == Byte ? 17 : 24); setAccumulator<size * 2>(DIVI<size>(getAccumulator<size * 2>(), mem)); break;
  }
}

template<u32 size> auto V30MZ::instructionGroup4MemImm() -> void {
  modRM();
  switch(modrm.reg) {
  case 0:  //INC
    wait(1);
    setMemory<size>(INC<size>(getMemory<size>()));
    break;
  case 1:  //DEC
    wait(1);
    setMemory<size>(DEC<size>(getMemory<size>()));
    break;
  case 2:  //CALL
    wait(5);
    push(PC);
    PC = getMemory<Word>();
    flush();
    break;
  case 3:  //CALLF
    wait(11);
    push(PS);
    push(PC);
    PC = getMemory<Word>(0);
    PS = getMemory<Word>(2);
    flush();
    break;
  case 4:  //JMP
    wait(4);
    PC = getMemory<Word>();
    flush();
    break;
  case 5:  //JMPF
    wait(9);
    PC = getMemory<Word>(0);
    PS = getMemory<Word>(2);
    flush();
    break;
  case 6:  //PUSH
    push(getMemory<Word>());
    break;
  case 7:  //PUSH (undocumented mirror)
    push(getMemory<Word>());
    break;
  }
}
