auto RSP::instructionADDIU() -> void {
  RT.u64 = i32(RS.u32 + IMMi16);
}

auto RSP::instructionADDU() -> void {
  RD.u64 = i32(RS.u32 + RT.u32);
}

auto RSP::instructionAND() -> void {
  RD.u64 = RS.u64 & RT.u64;
}

auto RSP::instructionANDI() -> void {
  RT.u64 = RS.u64 & IMMu16;
}

auto RSP::instructionB(bool take) -> void {
  if(take) IP = PC + (IMMi16 << 2);
}

auto RSP::instructionBREAK() -> void {
  scc.status.halted = 1;
  if(scc.status.interruptOnBreak) mi.irq.sp.line = 1;
}

auto RSP::instructionCACHE() -> void {
  //todo
}

auto RSP::instructionJ() -> void {
  IP = (PC & 0xf000'0000) | (IMMu26 << 2);
}

auto RSP::instructionJAL() -> void {
  IP = (PC & 0xf000'0000) | (IMMu26 << 2);
  RA.u64 = i32(PC + 4);
}

auto RSP::instructionJALR() -> void {
  IP = RS.u32;
  RA.u64 = i32(PC + 4);
}

auto RSP::instructionJR() -> void {
  IP = RS.u32;
}

auto RSP::instructionLB() -> void {
  if(auto data = readByte(RS.u32 + IMMi16)) RT.u64 = i8(*data);
}

auto RSP::instructionLBU() -> void {
  if(auto data = readByte(RS.u32 + IMMi16)) RT.u64 = u8(*data);
}

auto RSP::instructionLH() -> void {
  if(auto data = readHalf(RS.u32 + IMMi16)) RT.u64 = i16(*data);
}

auto RSP::instructionLHU() -> void {
  if(auto data = readHalf(RS.u32 + IMMi16)) RT.u64 = u16(*data);
}

auto RSP::instructionLUI() -> void {
  RT.u64 = i32(IMMu16 << 16);
}

auto RSP::instructionLW() -> void {
  if(auto data = readWord(RS.u32 + IMMi16)) RT.u64 = i32(*data);
}

auto RSP::instructionNOR() -> void {
  RD.u64 = ~(RS.u64 | RT.u64);
}

auto RSP::instructionOR() -> void {
  RD.u64 = RS.u64 | RT.u64;
}

auto RSP::instructionORI() -> void {
  RT.u64 = RS.u64 | IMMu16;
}

auto RSP::instructionSB() -> void {
  writeByte(RS.u32 + IMMi16, RT.u32);
}

auto RSP::instructionSH() -> void {
  writeHalf(RS.u32 + IMMi16, RT.u32);
}

auto RSP::instructionSLL() -> void {
  RD.u64 = i32(RT.u32 << SA);
}

auto RSP::instructionSLLV() -> void {
  RD.u64 = i32(RT.u32 << (RS.u32 & 31));
}

auto RSP::instructionSLTIU() -> void {
  RT.u64 = RS.u64 < u64(IMMu16);
}

auto RSP::instructionSLTU() -> void {
  RD.u64 = RS.u64 < RT.u64;
}

auto RSP::instructionSRA() -> void {
  RD.u64 = RT.i32 >> SA;
}

auto RSP::instructionSRAV() -> void {
  RD.u64 = RT.i32 >> (RS.u32 & 31);
}

auto RSP::instructionSRL() -> void {
  RD.u64 = i32(RT.u32 >> SA);
}

auto RSP::instructionSRLV() -> void {
  RD.u64 = i32(RT.u32 >> (RS.u32 & 31));
}

auto RSP::instructionSUBU() -> void {
  RD.u64 = i32(RS.u32 - RT.u32);
}

auto RSP::instructionSW() -> void {
  writeWord(RS.u32 + IMMi16, RT.u32);
}

auto RSP::instructionXOR() -> void {
  RD.u64 = RS.u64 ^ RT.u64;
}

auto RSP::instructionXORI() -> void {
  RS.u64 = RS.u64 ^ IMMu16;
}
