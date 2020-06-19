auto RSP::instructionADDIU(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = i32(rs.u32 + imm);
}

auto RSP::instructionADDU(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = i32(rs.u32 + rt.u32);
}

auto RSP::instructionAND(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = rs.u32 & rt.u32;
}

auto RSP::instructionANDI(r32& rt, cr32& rs, u16 imm) -> void {
  rt.u32 = rs.u32 & imm;
}

auto RSP::instructionBEQ(cr32& rs, cr32& rt, i16 imm) -> void {
  if(rs.u32 == rt.u32) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBGEZ(cr32& rs, i16 imm) -> void {
  if(rs.i32 >= 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBGEZAL(cr32& rs, i16 imm) -> void {
  RA.u32 = i32(PC + 8);
  if(rs.i32 >= 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBGTZ(cr32& rs, i16 imm) -> void {
  if(rs.i32 > 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBLEZ(cr32& rs, i16 imm) -> void {
  if(rs.i32 <= 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBLTZ(cr32& rs, i16 imm) -> void {
  if(rs.i32 < 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBLTZAL(cr32& rs, i16 imm) -> void {
  RA.u32 = i32(PC + 8);
  if(rs.i32 < 0) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBNE(cr32& rs, cr32& rt, i16 imm) -> void {
  if(rs.u32 != rt.u32) branch.take(PC + 4 + (imm << 2));
}

auto RSP::instructionBREAK() -> void {
  status.halted = 1;
  status.broken = 1;
  if(status.interruptOnBreak) mi.raise(MI::IRQ::SP);
  branch.halt();
}

auto RSP::instructionCACHE(u8 cache, u8 operation) -> void {
  //todo
}

auto RSP::instructionJ(u32 imm) -> void {
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto RSP::instructionJAL(u32 imm) -> void {
  RA.u32 = i32(PC + 8);
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto RSP::instructionJALR(r32& rd, cr32& rs) -> void {
  rd.u32 = i32(PC + 8);
  branch.take(rs.u32);
}

auto RSP::instructionJR(cr32& rs) -> void {
  branch.take(rs.u32);
}

auto RSP::instructionLB(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = i8(dmem.readByte(rs.u32 + imm));
}

auto RSP::instructionLBU(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = u8(dmem.readByte(rs.u32 + imm));
}

auto RSP::instructionLH(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = i16(dmem.readHalfUnaligned(rs.u32 + imm));
}

auto RSP::instructionLHU(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = u16(dmem.readHalfUnaligned(rs.u32 + imm));
}

auto RSP::instructionLUI(r32& rt, u16 imm) -> void {
  rt.u32 = i32(imm << 16);
}

auto RSP::instructionLW(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = i32(dmem.readWordUnaligned(rs.u32 + imm));
}

auto RSP::instructionNOR(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = ~(rs.u32 | rt.u32);
}

auto RSP::instructionOR(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = rs.u32 | rt.u32;
}

auto RSP::instructionORI(r32& rt, cr32& rs, u16 imm) -> void {
  rt.u32 = rs.u32 | imm;
}

auto RSP::instructionSB(cr32& rt, cr32& rs, i16 imm) -> void {
  dmem.writeByte(rs.u32 + imm, rt.u32);
}

auto RSP::instructionSH(cr32& rt, cr32& rs, i16 imm) -> void {
  dmem.writeHalfUnaligned(rs.u32 + imm, rt.u32);
}

auto RSP::instructionSLL(r32& rd, cr32& rt, u8 sa) -> void {
  rd.u32 = i32(rt.u32 << sa);
}

auto RSP::instructionSLLV(r32& rd, cr32& rt, cr32& rs) -> void {
  rd.u32 = i32(rt.u32 << (rs.u32 & 31));
}

auto RSP::instructionSLT(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = rs.i32 < rt.i32;
}

auto RSP::instructionSLTI(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = rs.i32 < imm;
}

auto RSP::instructionSLTIU(r32& rt, cr32& rs, i16 imm) -> void {
  rt.u32 = rs.u32 < imm;
}

auto RSP::instructionSLTU(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = rs.u32 < rt.u32;
}

auto RSP::instructionSRA(r32& rd, cr32& rt, u8 sa) -> void {
  rd.u32 = rt.i32 >> sa;
}

auto RSP::instructionSRAV(r32& rd, cr32& rt, cr32& rs) -> void {
  rd.u32 = rt.i32 >> (rs.u32 & 31);
}

auto RSP::instructionSRL(r32& rd, cr32& rt, u8 sa) -> void {
  rd.u32 = i32(rt.u32 >> sa);
}

auto RSP::instructionSRLV(r32& rd, cr32& rt, cr32& rs) -> void {
  rd.u32 = i32(rt.u32 >> (rs.u32 & 31));
}

auto RSP::instructionSUBU(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = i32(rs.u32 - rt.u32);
}

auto RSP::instructionSW(cr32& rt, cr32& rs, i16 imm) -> void {
  dmem.writeWordUnaligned(rs.u32 + imm, rt.u32);
}

auto RSP::instructionXOR(r32& rd, cr32& rs, cr32& rt) -> void {
  rd.u32 = rs.u32 ^ rt.u32;
}

auto RSP::instructionXORI(r32& rt, cr32& rs, u16 imm) -> void {
  rt.u32 = rs.u32 ^ imm;
}
