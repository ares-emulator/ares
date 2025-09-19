auto ARM7TDMI::armALU(n4 mode, n4 d, n32 rn, n32 rm) -> void {
  switch(mode) {
  case  0: r(d) = BIT(rn & rm); break;  //AND
  case  1: r(d) = BIT(rn ^ rm); break;  //EOR
  case  2: r(d) = SUB(rn, rm, 1); break;  //SUB
  case  3: r(d) = SUB(rm, rn, 1); break;  //RSB
  case  4: r(d) = ADD(rn, rm, 0); break;  //ADD
  case  5: r(d) = ADD(rn, rm, cpsr().c); break;  //ADC
  case  6: r(d) = SUB(rn, rm, cpsr().c); break;  //SBC
  case  7: r(d) = SUB(rm, rn, cpsr().c); break;  //RSC
  case  8:        BIT(rn & rm); break;  //TST
  case  9:        BIT(rn ^ rm); break;  //TEQ
  case 10:        SUB(rn, rm, 1); break;  //CMP
  case 11:        ADD(rn, rm, 0); break;  //CMN
  case 12: r(d) = BIT(rn | rm); break;  //ORR
  case 13: r(d) = BIT(rm); break;  //MOV
  case 14: r(d) = BIT(rn & ~rm); break;  //BIC
  case 15: r(d) = BIT(~rm); break;  //MVN
  }

  if(exception() && d == 15 && opcode.bit(20)) {
    cpsr() = spsr();
  }
}

auto ARM7TDMI::armMoveToStatus(n4 field, n1 mode, n32 data) -> void {
  if(mode) {
    spsr().set(field, data);
  } else {
    if(!privileged()) field.bit(0) = 0;
    cpsr().set(field, data);
    if(cpsr().t) r(15).data += 2;
  }
}

//

auto ARM7TDMI::armInstructionBranch
(i24 displacement, n1 link) -> void {
  if(link) r(14) = r(15) - 4;
  r(15) = r(15) + displacement * 4;
}

auto ARM7TDMI::armInstructionBranchExchangeRegister
(n4 m, n4 d, n4 field, n1 mode) -> void {
  n32 address = r(m);
  if((field & 0b1001) == 0b1001) {
    PSR& psr = mode ? spsr() : cpsr();
    psr.t = address.bit(0);
  } else {
    armMoveToStatus(field, mode, address);
  }
  r(d) = address;
}

auto ARM7TDMI::armInstructionCoprocessorDataProcessing
(n4 cm, n3 op2, n4 cpid, n4 cd, n4 cn, n4 op1) -> void {
  if(!CDP[cpid]) return armInstructionUndefined();
  CDP[cpid](cm, op2, cd, cn, op1);
}

auto ARM7TDMI::armInstructionDataImmediate
(n8 immediate, n4 shift, n4 d, n4 n, n1 save, n4 mode) -> void {
  n32 rn = r(n);
  n32 data = immediate;
  carry = cpsr().c;
  if(shift) data = ROR(data, shift << 1);
  armALU(mode, d, rn, data);
}

auto ARM7TDMI::armInstructionDataImmediateShift
(n4 m, n2 type, n5 shift, n4 d, n4 n, n1 save, n4 mode) -> void {
  n32 rn = r(n);
  n32 rm = r(m);
  carry = cpsr().c;

  switch(type) {
  case 0: rm = LSL(rm, shift); break;
  case 1: rm = LSR(rm, shift ? (u32)shift : 32); break;
  case 2: rm = ASR(rm, shift ? (u32)shift : 32); break;
  case 3: rm = shift ? ROR(rm, shift) : RRX(rm); break;
  }

  armALU(mode, d, rn, rm);
}

auto ARM7TDMI::armInstructionDataRegisterShift
(n4 m, n2 type, n4 s, n4 d, n4 n, n1 save, n4 mode) -> void {
  n8  rs = r(s);
  n32 rn = r(n) + (n == 15 ? 4 : 0);
  n32 rm = r(m) + (m == 15 ? 4 : 0);
  carry = cpsr().c;

  idle();
  switch(type) {
  case 0: rm = LSL(rm, rs); break;
  case 1: rm = LSR(rm, rs); break;
  case 2: rm = ASR(rm, rs); break;
  case 3: rm = ROR(rm, rs); break;
  }

  armALU(mode, d, rn, rm);
}

auto ARM7TDMI::armInstructionMemorySwap
(n4 m, n4 d, n4 n, n1 byte) -> void {
  lock();
  n32 word = load((byte ? Byte : Word), r(n));
  store((byte ? Byte : Word), r(n), r(m) + (m == 15 ? 4 : 0));
  unlock();
  idle();
  r(d) = word;
}

auto ARM7TDMI::armInstructionMoveHalfImmediate
(n8 immediate, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 rd = r(d) + (d == 15 ? 4 : 0);

  if(pre == 1) rn = up ? rn + immediate : rn - immediate;
  if(mode == 1) rd = load(Half, rn);
  if(mode == 0) store(Half, rn, rd);
  if(pre == 0) rn = up ? rn + immediate : rn - immediate;

  if(pre == 0 || writeback) r(n) = rn + (n == 15 ? 4 : 0);
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveHalfRegister
(n4 m, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 rm = r(m);
  n32 rd = r(d) + (d == 15 ? 4 : 0);

  if(pre == 1) rn = up ? rn + rm : rn - rm;
  if(mode == 1) rd = load(Half, rn);
  if(mode == 0) store(Half, rn, rd);
  if(pre == 0) rn = up ? rn + rm : rn - rm;

  if(pre == 0 || writeback) r(n) = rn + (n == 15 ? 4 : 0);
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveImmediateOffset
(n12 immediate, n4 d, n4 n, n1 mode, n1 writeback, n1 byte, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 rd = r(d) + (d == 15 ? 4 : 0);

  if(pre == 1) rn = up ? rn + immediate : rn - immediate;
  if(mode == 1) rd = load((byte ? Byte : Word), rn);
  if(mode == 0) store((byte ? Byte : Word), rn, rd);
  if(pre == 0) rn = up ? rn + immediate : rn - immediate;

  if(pre == 0 || writeback) r(n) = rn;
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveMultiple
(n16 list, n4 n, n1 mode, n1 writeback, n1 type, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 bitCount = list ? bit::count(list) : 16;
  n32 rnEnd;
  if(up == 1) rnEnd = r(n) + bitCount * 4;  //IA,IB
  if(up == 0) rnEnd = r(n) - bitCount * 4;  //DA,DB
  if(pre == 0 && up == 1) rn = rn + 0;  //IA
  if(pre == 1 && up == 1) rn = rn + 4;  //IB
  if(pre == 1 && up == 0) rn = rn - bitCount * 4 + 0;  //DB
  if(pre == 0 && up == 0) rn = rn - bitCount * 4 + 4;  //DA

  auto cpsrMode = cpsr().m;
  bool usr = false;
  if(type && mode == 1 && !list.bit(15)) usr = true;
  if(type && mode == 0) usr = true;
  if(usr) cpsr().m = PSR::USR;

  if(writeback && mode == 1 && !list.bit(n)) r(n) = rnEnd;

  endBurst();
  if(!list) list.bit(15) = 1;
  for(u32 m : range(16)) {
    if(!list.bit(m)) continue;
    if(mode == 1) r(m) = read(Word, rn);
    if(mode == 0) {
      write(Word, rn, r(m) + (!(n == 15 && writeback) && m == 15 ? 4 : 0));
      if(writeback) r(n) = rnEnd;  //writeback occurs after first access
    }
    rn += 4;
  }

  if(usr) cpsr().m = cpsrMode;

  if(mode) {
    idle();
    if(type && list.bit(15) && cpsr().m != PSR::USR && cpsr().m != PSR::SYS) {
      cpsr() = spsr();
    }
  } else {
    endBurst();
  }
}

auto ARM7TDMI::armInstructionMoveRegisterOffset
(n4 m, n2 type, n5 shift, n4 d, n4 n, n1 mode, n1 writeback, n1 byte, n1 up, n1 pre) -> void {
  n32 rm = r(m);
  n32 rd = r(d) + (d == 15 ? 4 : 0);
  n32 rn = r(n);
  carry = cpsr().c;

  switch(type) {
  case 0: rm = LSL(rm, shift); break;
  case 1: rm = LSR(rm, shift ? (u32)shift : 32); break;
  case 2: rm = ASR(rm, shift ? (u32)shift : 32); break;
  case 3: rm = shift ? ROR(rm, shift) : RRX(rm); break;
  }

  if(pre == 1) rn = up ? rn + rm : rn - rm;
  if(mode == 1) rd = load((byte ? Byte : Word), rn);
  if(mode == 0) store((byte ? Byte : Word), rn, rd);
  if(pre == 0) rn = up ? rn + rm : rn - rm;

  if(pre == 0 || writeback) r(n) = rn;
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveSignedImmediate
(n8 immediate, n1 half, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 rd = r(d) + (d == 15 ? 4 : 0);

  if(pre == 1) rn = up ? rn + immediate : rn - immediate;
  if(mode == 1) rd = load((half ? Half : Byte) | Signed, rn);
  if(mode == 0) store((half ? Half : Byte) | Signed, rn, rd);
  if(pre == 0) rn = up ? rn + immediate : rn - immediate;

  if(pre == 0 || writeback) r(n) = rn + (n == 15 ? 4 : 0);
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveSignedRegister
(n4 m, n1 half, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> void {
  n32 rn = r(n);
  n32 rm = r(m);
  n32 rd = r(d) + (d == 15 ? 4 : 0);

  if(pre == 1) rn = up ? rn + rm : rn - rm;
  if(mode == 1) rd = load((half ? Half : Byte) | Signed, rn);
  if(mode == 0) store((half ? Half : Byte) | Signed, rn, rd);
  if(pre == 0) rn = up ? rn + rm : rn - rm;

  if(pre == 0 || writeback) r(n) = rn + (n == 15 ? 4 : 0);
  if(mode == 1) {
    idle();
    r(d) = rd;
  }
}

auto ARM7TDMI::armInstructionMoveToCoprocessorFromRegister
(n4 cm, n3 op2, n4 cpid, n4 d, n4 cn, n3 op1) -> void {
  if(!MCR[cpid]) return armInstructionUndefined();
  MCR[cpid](r(d), cm, op2, cn, op1);
}

auto ARM7TDMI::armInstructionMoveToRegisterFromCoprocessor
(n4 cm, n3 op2, n4 cpid, n4 d, n4 cn, n3 op1) -> void {
  if(!MRC[cpid]) return armInstructionUndefined();
  r(d) = MRC[cpid](cm, op2, cn, op1);
}

auto ARM7TDMI::armInstructionMoveToRegisterFromRegister
(n4 d, n4 n) -> void {
  r(d) = r(n);
}

auto ARM7TDMI::armInstructionMoveToRegisterFromStatus
(n4 d, n1 mode) -> void {
  r(d) = (mode && cpsr().m != PSR::USR && cpsr().m != PSR::SYS) ? spsr() : cpsr();
}

auto ARM7TDMI::armInstructionMoveToStatusFromImmediate
(n8 immediate, n4 rotate, n4 field, n1 mode) -> void {
  n32 data = immediate;
  if(rotate) data = ROR(data, rotate << 1);
  armMoveToStatus(field, mode, data);
}

auto ARM7TDMI::armInstructionMoveToStatusFromRegister
(n4 m, n2 type, n5 shift, n4 field, n1 mode) -> void {
  n32 rm = r(m);
  carry = cpsr().c;

  switch(type) {
  case 0: rm = LSL(rm, shift); break;
  case 1: rm = LSR(rm, shift ? (u32)shift : 32); break;
  case 2: rm = ASR(rm, shift ? (u32)shift : 32); break;
  case 3: rm = shift ? ROR(rm, shift) : RRX(rm); break;
  }

  armMoveToStatus(field, mode, rm);
}

auto ARM7TDMI::armInstructionMultiply
(n4 m, n4 s, n4 n, n4 d, n1 save, n1 accumulate) -> void {
  if(accumulate) idle();
  n32 product = MUL(accumulate ? r(n) : 0, r(m), r(s));
  if(d != 15) r(d) = product;
}

auto ARM7TDMI::armInstructionMultiplyLong
(n4 m, n4 s, n4 l, n4 h, n1 save, n1 accumulate, n1 sign) -> void {
  n64 rm = r(m);
  n64 rs = r(s);

  idle();
  idle();
  if(accumulate) idle();

  if(sign) {
    if(rs >>  8 && rs >>  8 != 0xffffff) idle();
    if(rs >> 16 && rs >> 16 !=   0xffff) idle();
    if(rs >> 24 && rs >> 24 !=     0xff) idle();
    rm = (i32)rm;
    rs = (i32)rs;
  } else {
    if(rs >>  8) idle();
    if(rs >> 16) idle();
    if(rs >> 24) idle();
  }

  n64 rd = rm * rs;
  if(accumulate) rd += (n64)r(h) << 32 | (n64)r(l) << 0;

  if(l != 15) r(l) = rd >>  0;
  if(h != 15) r(h) = rd >> 32;

  if(save) {
    cpsr().z = rd == 0;
    cpsr().n = rd.bit(63);
  }
}

auto ARM7TDMI::armInstructionSoftwareInterrupt
(n24 immediate) -> void {
  exception(PSR::SVC, 0x08);
}

auto ARM7TDMI::armInstructionUndefined
() -> void {
  exception(PSR::UND, 0x04);
}
