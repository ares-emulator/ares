auto CPU::instructionCFC2(u32& rt, u8 rd) -> void {
  rt = gte.getControlRegister(rd);
}

auto CPU::instructionCTC2(cu32& rt, u8 rd) -> void {
  gte.setControlRegister(rd, rt);
}

auto CPU::instructionMFC2(u32& rt, u8 rd) -> void {
  rt = gte.getDataRegister(rd);
}

auto CPU::instructionMTC2(cu32& rt, u8 rd) -> void {
  gte.setDataRegister(rd, rt);
}

auto CPU::instructionLWC2(u8 rt, cu32& rs, i16 imm) -> void {
  auto data = readWord(rs + imm);
  gte.setDataRegister(rt, data);
}

auto CPU::instructionSWC2(u8 rt, cu32& rs, i16 imm) -> void {
  auto data = gte.getDataRegister(rt);
  writeWord(rs + imm, data);
}

//

auto CPU::instructionAVSZ3() -> void {
  gte.clearFlags();
  gte.avsz3();
  gte.updateError();
}

auto CPU::instructionAVSZ4() -> void {
  gte.clearFlags();
  gte.avsz4();
  gte.updateError();
}

auto CPU::instructionCC() -> void {
  gte.clearFlags();
  gte.cc();
  gte.updateError();
}

auto CPU::instructionCDP() -> void {
  gte.clearFlags();
  gte.cdp();
  gte.updateError();
}

auto CPU::instructionDCPL() -> void {
  gte.clearFlags();
  gte.dcpl();
  gte.updateError();
}

auto CPU::instructionDPCS() -> void {
  gte.clearFlags();
  gte.dpcs();
  gte.updateError();
}

auto CPU::instructionDPCT() -> void {
  gte.clearFlags();
  gte.dpct();
  gte.updateError();
}

auto CPU::instructionGPF() -> void {
  gte.clearFlags();
  gte.gpf();
  gte.updateError();
}

auto CPU::instructionGPL() -> void {
  gte.clearFlags();
  gte.gpl();
  gte.updateError();
}

auto CPU::instructionINTPL() -> void {
  gte.clearFlags();
  gte.intpl();
  gte.updateError();
}

auto CPU::instructionMVMVA() -> void {
  gte.clearFlags();
  gte.mvmva();
  gte.updateError();
}

auto CPU::instructionNCCS() -> void {
  gte.clearFlags();
  gte.nccs();
  gte.updateError();
}

auto CPU::instructionNCCT() -> void {
  gte.clearFlags();
  gte.ncct();
  gte.updateError();
}

auto CPU::instructionNCDS(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.ncds();
  gte.updateError();
}

auto CPU::instructionNCDT(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.ncdt();
  gte.updateError();
}

auto CPU::instructionNCLIP() -> void {
  gte.clearFlags();
  gte.nclip();
  gte.updateError();
}

auto CPU::instructionNCS() -> void {
  gte.clearFlags();
  gte.ncs();
  gte.updateError();
}

auto CPU::instructionNCT() -> void {
  gte.clearFlags();
  gte.nct();
  gte.updateError();
}

auto CPU::instructionOP() -> void {
  gte.clearFlags();
  gte.op();
  gte.updateError();
}

auto CPU::instructionRTPS(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.rtps();
  gte.updateError();
}

auto CPU::instructionRTPT(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.rtpt();
  gte.updateError();
}

auto CPU::instructionSQR() -> void {
  gte.clearFlags();
  gte.sqr();
  gte.updateError();
}
