auto ARM7TDMI::thumbInstructionALU
(n3 d, n3 m, n4 mode) -> void {
  carry = cpsr().c;
  switch(mode) {
  case  0: r(d) = BIT(r(d) & r(m)); break;  //AND
  case  1: r(d) = BIT(r(d) ^ r(m)); break;  //EOR
  case  2: r(d) = BIT(LSL(r(d), r(m))); idle(); break;  //LSL
  case  3: r(d) = BIT(LSR(r(d), r(m))); idle(); break;  //LSR
  case  4: r(d) = BIT(ASR(r(d), r(m))); idle(); break;  //ASR
  case  5: r(d) = ADD(r(d), r(m), cpsr().c); break;  //ADC
  case  6: r(d) = SUB(r(d), r(m), cpsr().c); break;  //SBC
  case  7: r(d) = BIT(ROR(r(d), r(m))); idle(); break;  //ROR
  case  8:        BIT(r(d) & r(m)); break;  //TST
  case  9: r(d) = SUB(0, r(m), 1); break;  //NEG
  case 10:        SUB(r(d), r(m), 1); break;  //CMP
  case 11:        ADD(r(d), r(m), 0); break;  //CMN
  case 12: r(d) = BIT(r(d) | r(m)); break;  //ORR
  case 13: r(d) = MUL(0, r(m), r(d)); break;  //MUL
  case 14: r(d) = BIT(r(d) & ~r(m)); break;  //BIC
  case 15: r(d) = BIT(~r(m)); break;  //MVN
  }
}

auto ARM7TDMI::thumbInstructionALUExtended
(n4 d, n4 m, n2 mode) -> void {
  switch(mode) {
  case 0: r(d) = r(d) + r(m); break;  //ADD
  case 1: SUB(r(d), r(m), 1); break;  //CMP
  case 2: r(d) = r(m); break;  //MOV
  }
}

auto ARM7TDMI::thumbInstructionAddRegister
(n8 immediate, n3 d, n1 mode) -> void {
  switch(mode) {
  case 0: r(d) = (r(15) & ~3) + immediate * 4; break;  //ADD pc
  case 1: r(d) = r(13) + immediate * 4; break;  //ADD sp
  }
}

auto ARM7TDMI::thumbInstructionAdjustImmediate
(n3 d, n3 n, n3 immediate, n1 mode) -> void {
  switch(mode) {
  case 0: r(d) = ADD(r(n), immediate, 0); break;  //ADD
  case 1: r(d) = SUB(r(n), immediate, 1); break;  //SUB
  }
}

auto ARM7TDMI::thumbInstructionAdjustRegister
(n3 d, n3 n, n3 m, n1 mode) -> void {
  switch(mode) {
  case 0: r(d) = ADD(r(n), r(m), 0); break;  //ADD
  case 1: r(d) = SUB(r(n), r(m), 1); break;  //SUB
  }
}

auto ARM7TDMI::thumbInstructionAdjustStack
(n7 immediate, n1 mode) -> void {
  switch(mode) {
  case 0: r(13) = r(13) + immediate * 4; break;  //ADD
  case 1: r(13) = r(13) - immediate * 4; break;  //SUB
  }
}

auto ARM7TDMI::thumbInstructionBranchExchange
(n4 m) -> void {
  n32 address = r(m);
  cpsr().t = address.bit(0);
  r(15) = address;
}

auto ARM7TDMI::thumbInstructionBranchFarPrefix
(i11 displacement) -> void {
  r(14) = r(15) + (displacement * 2 << 11);
}

auto ARM7TDMI::thumbInstructionBranchFarSuffix
(n11 displacement) -> void {
  r(15) = r(14) + (displacement * 2);
  r(14) = pipeline.decode.address | 1;
}

auto ARM7TDMI::thumbInstructionBranchNear
(i11 displacement) -> void {
  r(15) = r(15) + displacement * 2;
}

auto ARM7TDMI::thumbInstructionBranchTest
(i8 displacement, n4 condition) -> void {
  if(!TST(condition)) return;
  r(15) = r(15) + displacement * 2;
}

auto ARM7TDMI::thumbInstructionImmediate
(n8 immediate, n3 d, n2 mode) -> void {
  carry = cpsr().c;
  switch(mode) {
  case 0: r(d) = BIT(immediate); break;  //MOV
  case 1:        SUB(r(d), immediate, 1); break;  //CMP
  case 2: r(d) = ADD(r(d), immediate, 0); break;  //ADD
  case 3: r(d) = SUB(r(d), immediate, 1); break;  //SUB
  }
}

auto ARM7TDMI::thumbInstructionLoadLiteral
(n8 displacement, n3 d) -> void {
  n32 address = (r(15) & ~3) + (displacement << 2);
  r(d) = load(Word, address);
  idle();
}

auto ARM7TDMI::thumbInstructionMoveByteImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> void {
  switch(mode) {
  case 0: store(Byte, r(n) + offset, r(d)); break;  //STRB
  case 1: r(d) = load(Byte, r(n) + offset); idle(); break;  //LDRB
  }
}

auto ARM7TDMI::thumbInstructionMoveHalfImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> void {
  switch(mode) {
  case 0: store(Half, r(n) + offset * 2, r(d)); break;  //STRH
  case 1: r(d) = load(Half, r(n) + offset * 2); idle(); break;  //LDRH
  }
}

auto ARM7TDMI::thumbInstructionMoveMultiple
(n8 list, n3 n, n1 mode) -> void {
  n32 rn = r(n);
  n16 rlist = list;
  n32 bitCount = rlist ? bit::count(rlist) : 16;
  n32 rnEnd = r(n) + bitCount * 4;

  if(mode == 1 && !rlist.bit(n)) r(n) = rnEnd;

  endBurst();
  if(!rlist) rlist.bit(15) = 1;
  for(u32 m : range(16)) {
    if(!rlist.bit(m)) continue;
    if(mode == 1) r(m) = read(Word, rn);  //LDMIA
    if(mode == 0) {
      write(Word, rn, r(m) + (m == 15 ? 2 : 0));  //STMIA
      r(n) = rnEnd;  //writeback occurs after first access
    }
    rn += 4;
  }

  if(mode) {
    idle();
  } else {
    endBurst();
  }
}

auto ARM7TDMI::thumbInstructionMoveRegisterOffset
(n3 d, n3 n, n3 m, n3 mode) -> void {
  switch(mode) {
  case 0: store(Word, r(n) + r(m), r(d)); break;  //STR
  case 1: store(Half, r(n) + r(m), r(d)); break;  //STRH
  case 2: store(Byte, r(n) + r(m), r(d)); break;  //STRB
  case 3: r(d) = load(Byte | Signed, r(n) + r(m)); idle(); break;  //LDSB
  case 4: r(d) = load(Word, r(n) + r(m)); idle(); break;  //LDR
  case 5: r(d) = load(Half, r(n) + r(m)); idle(); break;  //LDRH
  case 6: r(d) = load(Byte, r(n) + r(m)); idle(); break;  //LDRB
  case 7: r(d) = load(Half | Signed, r(n) + r(m)); idle(); break;  //LDSH
  }
}

auto ARM7TDMI::thumbInstructionMoveStack
(n8 immediate, n3 d, n1 mode) -> void {
  switch(mode) {
  case 0: store(Word, r(13) + immediate * 4, r(d)); break;  //STR
  case 1: r(d) = load(Word, r(13) + immediate * 4); idle(); break;  //LDR
  }
}

auto ARM7TDMI::thumbInstructionMoveWordImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> void {
  switch(mode) {
  case 0: store(Word, r(n) + offset * 4, r(d)); break;  //STR
  case 1: r(d) = load(Word, r(n) + offset * 4); idle(); break;  //LDR
  }
}

auto ARM7TDMI::thumbInstructionShiftImmediate
(n3 d, n3 m, n5 immediate, n2 mode) -> void {
  switch(mode) {
  case 0: r(d) = BIT(LSL(r(m), immediate)); break;  //LSL
  case 1: r(d) = BIT(LSR(r(m), immediate ? (u32)immediate : 32)); break;  //LSR
  case 2: r(d) = BIT(ASR(r(m), immediate ? (u32)immediate : 32)); break;  //ASR
  }
}

auto ARM7TDMI::thumbInstructionSoftwareInterrupt
(n8 immediate) -> void {
  exception(PSR::SVC, 0x08);
}

auto ARM7TDMI::thumbInstructionStackMultiple
(n8 list, n1 lrpc, n1 mode) -> void {
  n32 sp = r(13);
  n16 rlist = list;
  if(lrpc) {
    if(mode == 1) rlist.bit(15) = 1;  //POP
    if(mode == 0) rlist.bit(14) = 1;  //PUSH
  }
  n32 bitCount = rlist ? bit::count(rlist) : 16;
  if(mode == 1) r(13) = r(13) + bitCount * 4;  //POP
  if(mode == 0) sp = sp - bitCount * 4 + 0;    //PUSH

  endBurst();
  if(!rlist) rlist.bit(15) = 1;
  for(u32 m : range(16)) {
    if(!rlist.bit(m)) continue;
    if(mode == 1) r(m) = read(Word, sp);  //POP
    if(mode == 0) write(Word, sp, r(m) + (m == 15 ? 2 : 0));  //PUSH
    sp += 4;
  }

  if(mode) {
    idle();
  } else {
    endBurst();
    r(13) = r(13) - bitCount * 4;  //PUSH
  }
}

auto ARM7TDMI::thumbInstructionUndefined
() -> void {
  exception(PSR::UND, 0x04);
}
