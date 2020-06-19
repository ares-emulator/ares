#define PC core.pc
#define RA core.r[Core::Register::RA]
#define HI core.hi
#define LO core.lo

auto CPU::instructionADD(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs + rt;
}

auto CPU::instructionADDI(u32& rt, cu32& rs, i16 imm) -> void {
  rt = rs + imm;
}

auto CPU::instructionADDIU(u32& rt, cu32& rs, i16 imm) -> void {
  rt = rs + imm;
}

auto CPU::instructionADDU(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs + rt;
}

auto CPU::instructionAND(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs & rt;
}

auto CPU::instructionANDI(u32& rt, cu32& rs, u16 imm) -> void {
  rt = rs & imm;
}

auto CPU::instructionBEQ(cu32& rs, cu32& rt, i16 imm) -> void {
  if(rs == rt) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGEZ(ci32& rs, i16 imm) -> void {
  if(rs >= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGEZAL(ci32& rs, i16 imm) -> void {
  RA = PC + 8;
  if(rs >= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGTZ(ci32& rs, i16 imm) -> void {
  if(rs > 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLEZ(ci32& rs, i16 imm) -> void {
  if(rs <= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLTZ(ci32& rs, i16 imm) -> void {
  if(rs < 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLTZAL(ci32& rs, i16 imm) -> void {
  RA = PC + 8;
  if(rs < 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBNE(cu32& rs, cu32& rt, i16 imm) -> void {
  if(rs != rt) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBREAK() -> void {
  exception.breakpoint();
}

auto CPU::instructionDIV(ci32& rs, ci32& rt) -> void {
  if(rt) {
    //cast to i64 to prevent exception on INT32_MIN / -1
    LO = i64(rs) / i64(rt);
    HI = i64(rs) % i64(rt);
  } else {
    LO = rs < 0 ? +1 : -1;
    HI = rs;
  }
}

auto CPU::instructionDIVU(cu32& rs, cu32& rt) -> void {
  if(rt) {
    LO = rs / rt;
    HI = rs % rt;
  } else {
    LO = -1;
    HI = rs;
  }
}

auto CPU::instructionJ(u32 imm) -> void {
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto CPU::instructionJAL(u32 imm) -> void {
  RA = PC + 8;
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto CPU::instructionJALR(u32& rd, cu32& rs) -> void {
  rd = PC + 8;
  branch.take(rs);
}

auto CPU::instructionJR(cu32& rs) -> void {
  branch.take(rs);
}

auto CPU::instructionLB(u32& rt, cu32& rs, i16 imm) -> void {
  if(auto data = readByte(rs + imm)) rt = i8(*data);
}

auto CPU::instructionLBU(u32& rt, cu32& rs, i16 imm) -> void {
  if(auto data = readByte(rs + imm)) rt = u8(*data);
}

auto CPU::instructionLH(u32& rt, cu32& rs, i16 imm) -> void {
  if(auto data = readHalf(rs + imm)) rt = i16(*data);
}

auto CPU::instructionLHU(u32& rt, cu32& rs, i16 imm) -> void {
  if(auto data = readHalf(rs + imm)) rt = u16(*data);
}

auto CPU::instructionLUI(u32& rt, u16 imm) -> void {
  rt = imm << 16;
}

auto CPU::instructionLW(u32& rt, cu32& rs, i16 imm) -> void {
  if(auto data = readWord(rs + imm)) rt = i32(*data);
}

auto CPU::instructionLWL(u32& rt, cu32& rs, i16 imm) -> void {
  auto address = rs + imm;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = readWord(address & ~3)) {
    rt = rt & ~mask | *data << shift;
  }
}

auto CPU::instructionLWR(u32& rt, cu32& rs, i16 imm) -> void {
  auto address = rs + imm;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = readWord(address & ~3)) {
    rt = rt & ~mask | *data >> shift;
  }
}

auto CPU::instructionMFHI(u32& rd) -> void {
  rd = HI;
}

auto CPU::instructionMFLO(u32& rd) -> void {
  rd = LO;
}

auto CPU::instructionMTHI(cu32& rs) -> void {
  HI = rs;
}

auto CPU::instructionMTLO(cu32& rs) -> void {
  LO = rs;
}

auto CPU::instructionMULT(ci32& rs, ci32& rt) -> void {
  u64 result = i64(rs) * i64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::instructionMULTU(cu32& rs, cu32& rt) -> void {
  u64 result = u64(rs) * u64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::instructionNOR(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = ~(rs | rt);
}

auto CPU::instructionOR(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs | rt;
}

auto CPU::instructionORI(u32& rt, cu32& rs, u16 imm) -> void {
  rt = rs | imm;
}

auto CPU::instructionSB(cu32& rt, cu32& rs, i16 imm) -> void {
  writeByte(rs + imm, rt);
}

auto CPU::instructionSH(cu32& rt, cu32& rs, i16 imm) -> void {
  writeHalf(rs + imm, rt);
}

auto CPU::instructionSLL(u32& rd, cu32& rt, u8 sa) -> void {
  rd = rt << sa;
}

auto CPU::instructionSLLV(u32& rd, cu32& rt, cu32& rs) -> void {
  rd = rt << (rs & 31);
}

auto CPU::instructionSLT(u32& rd, ci32& rs, ci32& rt) -> void {
  rd = rs < rt;
}

auto CPU::instructionSLTI(u32& rt, ci32& rs, i16 imm) -> void {
  rt = rs < imm;
}

auto CPU::instructionSLTIU(u32& rt, cu32& rs, i16 imm) -> void {
  rt = rs < imm;
}

auto CPU::instructionSLTU(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs < rt;
}

auto CPU::instructionSRA(u32& rd, ci32& rt, u8 sa) -> void {
  rd = rt >> sa;
}

auto CPU::instructionSRAV(u32& rd, ci32& rt, cu32& rs) -> void {
  rd = rt >> (rs & 31);
}

auto CPU::instructionSRL(u32& rd, cu32& rt, u8 sa) -> void {
  rd = rt >> sa;
}

auto CPU::instructionSRLV(u32& rd, cu32& rt, cu32& rs) -> void {
  rd = rt >> (rs & 31);
}

auto CPU::instructionSUB(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs - rt;
}

auto CPU::instructionSUBU(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs - rt;
}

auto CPU::instructionSW(cu32& rt, cu32& rs, i16 imm) -> void {
  writeWord(rs + imm, rt);
}

auto CPU::instructionSWL(cu32& rt, cu32& rs, i16 imm) -> void {
  auto address = rs + imm;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = readWord(address & ~3)) {
    writeWord(address & ~3, *data & ~mask | rt >> shift);
  }
}

auto CPU::instructionSWR(cu32& rt, cu32& rs, i16 imm) -> void {
  auto address = rs + imm;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = readWord(address & ~3)) {
    writeWord(address & ~3, *data & ~mask | rt << shift);
  }
}

auto CPU::instructionSYSCALL() -> void {
  exception.systemCall();
}

auto CPU::instructionXOR(u32& rd, cu32& rs, cu32& rt) -> void {
  rd = rs ^ rt;
}

auto CPU::instructionXORI(u32& rt, cu32& rs, u16 imm) -> void {
  rt = rs ^ imm;
}

#undef PC
#undef RA
#undef HI
#undef LO
