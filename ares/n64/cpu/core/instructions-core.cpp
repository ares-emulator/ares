auto CPU::instructionADD() -> void {
//if(RS.u32 > ~RT.u32) return exception(Overflow);
  RD.u64 = i32(RS.u32 + RT.u32);
}

auto CPU::instructionADDI() -> void {
//if(RS.i32 > ~IMM16i) return exception(Overflow);
  RT.u64 = i32(RS.u32 + IMMi16);
}

auto CPU::instructionADDIU() -> void {
  RT.u64 = i32(RS.u32 + IMMi16);
}

auto CPU::instructionADDU() -> void {
  RD.u64 = i32(RS.u32 + RT.u32);
}

auto CPU::instructionAND() -> void {
  RD.u64 = RS.u64 & RT.u64;
}

auto CPU::instructionANDI() -> void {
  RT.u64 = RS.u64 & IMMu16;
}

auto CPU::instructionB(bool take) -> void {
  if(take) IP = PC + (IMMi16 << 2);
}

auto CPU::instructionBAL(bool take) -> void {
  RA.u64 = i32(PC + 4);
  if(take) IP = PC + (IMMi16 << 2);
}

auto CPU::instructionBALL(bool take) -> void {
  RA.u64 = i32(PC + 4);
  if(take) IP = PC + (IMMi16 << 2);
  else PC += 4;
}

auto CPU::instructionBL(bool take) -> void {
  if(take) IP = PC + (IMMi16 << 2);
  else PC += 4;
}

auto CPU::instructionBREAK() -> void {
  exception(Break);
}

auto CPU::instructionCACHE() -> void {
  //todo
}

auto CPU::instructionDADD() -> void {
//if(RS.u64 > ~RT.u64) return exception(Overflow);
  RD.u64 = RS.u64 + RT.u64;
}

auto CPU::instructionDADDI() -> void {
//if(RS.i64 > ~IMM16i) return exception(Overflow);
  RT.u64 = RS.u64 + i64(IMMi16);
}

auto CPU::instructionDADDIU() -> void {
  RT.u64 = RS.u64 + u64(IMMi16);
}

auto CPU::instructionDADDU() -> void {
  RD.u64 = RS.u64 + RT.u64;
}

auto CPU::instructionDDIV() -> void {
  if(RT.i64) {
    //cast to i128 to prevent exception on INT64_MIN / -1
    LO.u64 = (i128)RS.i64 / (i128)RT.i64;
    HI.u64 = (i128)RS.i64 % (i128)RT.i64;
  } else {
    LO.u64 = RS.i64 < 0 ? +1 : -1;
    HI.u64 = RS.i64;
  }
}

auto CPU::instructionDDIVU() -> void {
  if(RT.u64) {
    LO.u64 = RS.u64 / RT.u64;
    HI.u64 = RS.u64 % RT.u64;
  } else {
    LO.u64 = -1;
    HI.u64 = RS.u64;
  }
}

auto CPU::instructionDIV() -> void {
  if(RT.i32) {
    //cast to i64 to prevent exception on INT32_MIN / -1
    LO.u64 = i32((i64)RS.i32 / (i64)RT.i32);
    HI.u64 = i32((i64)RS.i32 % (i64)RT.i32);
  } else {
    LO.u64 = RS.i32 < 0 ? +1 : -1;
    HI.u64 = RS.i32;
  }
}

auto CPU::instructionDIVU() -> void {
  if(RT.u32) {
    LO.u64 = i32(RS.u32 / RT.u32);
    HI.u64 = i32(RS.u32 % RT.u32);
  } else {
    LO.u64 = -1;
    HI.u64 = RS.i32;
  }
}

auto CPU::instructionDMULT() -> void {
  auto result = RS.i128() * RT.i128();
  LO.u64 = result >>  0;
  HI.u64 = result >> 64;
}

auto CPU::instructionDMULTU() -> void {
  auto result = RS.u128() * RT.u128();
  LO.u64 = result >>  0;
  HI.u64 = result >> 64;
}

auto CPU::instructionDSLL() -> void {
  RD.u64 = RT.u64 << SA;
}

auto CPU::instructionDSLL32() -> void {
  RD.u64 = RT.u64 << SA + 32;
}

auto CPU::instructionDSLLV() -> void {
  RD.u64 = RT.u64 << (RS.u32 & 63);
}

auto CPU::instructionDSRA() -> void {
  RD.u64 = RT.i64 >> SA;
}

auto CPU::instructionDSRA32() -> void {
  RD.u64 = RT.i64 >> SA + 32;
}

auto CPU::instructionDSRAV() -> void {
  RD.u64 = RT.i64 >> (RS.u32 & 63);
}

auto CPU::instructionDSRL() -> void {
  RD.u64 = RT.u64 >> SA;
}

auto CPU::instructionDSRL32() -> void {
  RD.u64 = RT.u64 >> SA + 32;
}

auto CPU::instructionDSRLV() -> void {
  RD.u64 = RT.u64 >> (RS.u32 & 63);
}

auto CPU::instructionDSUB() -> void {
//if(RS.u64 < RT.u64) return exception(Overflow);
  RD.u64 = RS.u64 - RT.u64;
}

auto CPU::instructionDSUBU() -> void {
  RD.u64 = RS.u64 - RT.u64;
}

auto CPU::instructionJ() -> void {
  IP = (PC & 0xf000'0000) | (IMMu26 << 2);
}

auto CPU::instructionJAL() -> void {
  IP = (PC & 0xf000'0000) | (IMMu26 << 2);
  RA.u64 = i32(PC + 4);
}

auto CPU::instructionJALR() -> void {
  IP = RS.u32;
  RA.u64 = i32(PC + 4);
}

auto CPU::instructionJR() -> void {
  IP = RS.u32;
}

auto CPU::instructionLB() -> void {
  if(auto data = readByte(RS.u32 + IMMi16)) RT.u64 = i8(*data);
}

auto CPU::instructionLBU() -> void {
  if(auto data = readByte(RS.u32 + IMMi16)) RT.u64 = u8(*data);
}

auto CPU::instructionLD() -> void {
  if(auto data = readDouble(RS.u32 + IMMi16)) RT.u64 = *data;
}

auto CPU::instructionLDC1() -> void {
  if(!STATUS_COP1) return exception(CoprocessorUnusable);
  if(auto data = readDouble(RS.u32 + IMMi16)) cop1.r[RTn].u64 = *data;
}

auto CPU::instructionLDC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  exception(InvalidInstruction);  //coprocessor 2 not present
}

auto CPU::instructionLDL() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = u64(0) - 1 << shift;
  if(auto data = readDouble(address & ~7)) {
    RT.u64 = RT.u64 & ~mask | *data << shift;
  }
}

auto CPU::instructionLDR() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = u64(0) - 1 >> shift;
  if(auto data = readDouble(address & ~7)) {
    RT.u64 = RT.u64 & ~mask | *data >> shift;
  }
}

auto CPU::instructionLH() -> void {
  if(auto data = readHalf(RS.u32 + IMMi16)) RT.u64 = i16(*data);
}

auto CPU::instructionLHU() -> void {
  if(auto data = readHalf(RS.u32 + IMMi16)) RT.u64 = u16(*data);
}

auto CPU::instructionLL() -> void {
  if(auto data = readWord(RS.u32 + IMMi16)) {
    RT.u64 = *data;
    LL = RS.u32 + IMMi16;
    LLBIT = 1;
  }
}

auto CPU::instructionLLD() -> void {
  if(auto data = readDouble(RS.u32 + IMMi16)) {
    RT.u64 = *data;
    LL = RS.u32 + IMMi16;
    LLBIT = 1;
  }
}

auto CPU::instructionLUI() -> void {
  RT.u64 = i32(IMMu16 << 16);
}

auto CPU::instructionLW() -> void {
  if(auto data = readWord(RS.u32 + IMMi16)) RT.u64 = i32(*data);
}

auto CPU::instructionLWL() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = readWord(address & ~3)) {
    RT.u64 = i32(RT.u32 & ~mask | *data << shift);
  }
}

auto CPU::instructionLWR() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = readWord(address & ~3)) {
    RT.u64 = i32(RT.u32 & ~mask | *data >> shift);
  }
}

auto CPU::instructionLWU() -> void {
  if(auto data = readWord(RS.u32 + IMMi16)) RT.u64 = u32(*data);
}

auto CPU::instructionMFHI() -> void {
  RD.u64 = HI.u64;
}

auto CPU::instructionMFLO() -> void {
  RD.u64 = LO.u64;
}

auto CPU::instructionMTHI() -> void {
  HI.u64 = RS.u64;
}

auto CPU::instructionMTLO() -> void {
  LO.u64 = RS.u64;
}

auto CPU::instructionMULT() -> void {
  auto result = i64(RS.i32) * i64(RT.i32);
  LO.u64 = i32(result >>  0);
  HI.u64 = i32(result >> 32);
}

auto CPU::instructionMULTU() -> void {
  auto result = u64(RS.u32) * u64(RT.u32);
  LO.u64 = i32(result >>  0);
  HI.u64 = i32(result >> 32);
}

auto CPU::instructionNOR() -> void {
  RD.u64 = ~(RS.u64 | RT.u64);
}

auto CPU::instructionOR() -> void {
  RD.u64 = RS.u64 | RT.u64;
}

auto CPU::instructionORI() -> void {
  RT.u64 = RS.u64 | IMMu16;
}

auto CPU::instructionSB() -> void {
  writeByte(RS.u32 + IMMi16, RT.u32);
}

auto CPU::instructionSC() -> void {
  u32 address = RS.u32 + IMMi16;
  if(readWord(address) && RTn && LL == address) {
    writeWord(address, RT.u32);
    RT.u64 = 1;
  } else {
    RT.u64 = 0;
  }
}

auto CPU::instructionSCD() -> void {
  u32 address = RS.u32 + IMMi16;
  if(readDouble(address) && RTn && LL == address) {
    writeDouble(address, RT.u64);
    RT.u64 = 1;
  } else {
    RT.u64 = 0;
  }
}

auto CPU::instructionSD() -> void {
  writeDouble(RS.u32 + IMMi16, RT.u64);
}

auto CPU::instructionSDC1() -> void {
  if(!STATUS_COP1) return exception(CoprocessorUnusable);
  writeDouble(RS.u32 + IMMi16, cop1.r[RTn].u64);
}

auto CPU::instructionSDC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  exception(InvalidInstruction);  //coprocessor 2 not present
}

auto CPU::instructionSDL() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = u64(0) - 1 >> shift;
  if(auto data = readDouble(address & ~7)) {
    writeDouble(address & ~7, *data & ~mask | RT.u64 >> shift);
  }
}

auto CPU::instructionSDR() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = u64(0) - 1 << shift;
  if(auto data = readDouble(address & ~7)) {
    writeDouble(address & ~7, *data & ~mask | RT.u64 << shift);
  }
}

auto CPU::instructionSH() -> void {
  writeHalf(RS.u32 + IMMi16, RT.u32);
}

auto CPU::instructionSLL() -> void {
  RD.u64 = i32(RT.u32 << SA);
}

auto CPU::instructionSLLV() -> void {
  RD.u64 = i32(RT.u32 << (RS.u32 & 31));
}

auto CPU::instructionSLT() -> void {
  RD.u64 = RS.i64 < RT.i64;
}

auto CPU::instructionSLTI() -> void {
  RT.u64 = RS.i64 < i64(IMMi16);
}

auto CPU::instructionSLTIU() -> void {
  RT.u64 = RS.u64 < u64(IMMu16);
}

auto CPU::instructionSLTU() -> void {
  RD.u64 = RS.u64 < RT.u64;
}

auto CPU::instructionSRA() -> void {
  RD.u64 = RT.i32 >> SA;
}

auto CPU::instructionSRAV() -> void {
  RD.u64 = RT.i32 >> (RS.u32 & 31);
}

auto CPU::instructionSRL() -> void {
  RD.u64 = i32(RT.u32 >> SA);
}

auto CPU::instructionSRLV() -> void {
  RD.u64 = i32(RT.u32 >> (RS.u32 & 31));
}

auto CPU::instructionSUB() -> void {
//if(RS.u32 < RT.u32) return exception(Overflow);
  RD.u64 = i32(RS.u32 - RT.u32);
}

auto CPU::instructionSUBU() -> void {
  RD.u64 = i32(RS.u32 - RT.u32);
}

auto CPU::instructionSW() -> void {
  writeWord(RS.u32 + IMMi16, RT.u32);
}

auto CPU::instructionSWL() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = readWord(address & ~3)) {
    writeWord(address & ~3, *data & ~mask | RT.u32 >> shift);
  }
}

auto CPU::instructionSWR() -> void {
  auto address = RS.u32 + IMMi16;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = readWord(address & ~3)) {
    writeWord(address & ~3, *data & ~mask | RT.u32 << shift);
  }
}

auto CPU::instructionSYSCALL() -> void {
  exception(Syscall);
}

auto CPU::instructionTEQ() -> void {
  if(RS.u64 == RT.u64) exception(Trap);
}

auto CPU::instructionTEQI() -> void {
  if(RS.u64 == IMMu16) exception(Trap);
}

auto CPU::instructionTGE() -> void {
  if(RS.i64 >= RT.i64) exception(Trap);
}

auto CPU::instructionTGEI() -> void {
  if(RS.i64 >= IMMi16) exception(Trap);
}

auto CPU::instructionTGEIU() -> void {
  if(RS.u64 >= IMMu16) exception(Trap);
}

auto CPU::instructionTGEU() -> void {
  if(RS.u64 >= RT.u64) exception(Trap);
}

auto CPU::instructionTLT() -> void {
  if(RS.i64 < RT.i64) exception(Trap);
}

auto CPU::instructionTLTI() -> void {
  if(RS.i64 < IMMi16) exception(Trap);
}

auto CPU::instructionTLTIU() -> void {
  if(RS.u64 < IMMu16) exception(Trap);
}

auto CPU::instructionTLTU() -> void {
  if(RS.u64 < RT.u64) exception(Trap);
}

auto CPU::instructionTNE() -> void {
  if(RS.u64 != RT.u64) exception(Trap);
}

auto CPU::instructionTNEI() -> void {
  if(RS.u64 != IMMu16) exception(Trap);
}

auto CPU::instructionXOR() -> void {
  RD.u64 = RS.u64 ^ RT.u64;
}

auto CPU::instructionXORI() -> void {
  RT.u64 = RS.u64 ^ IMMu16;
}
