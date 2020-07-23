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

auto CPU::instructionCC(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.cc();
  gte.updateError();
}

auto CPU::instructionCDP(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.cdp();
  gte.updateError();
}

auto CPU::instructionDCPL(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.dcpl();
  gte.updateError();
}

auto CPU::instructionDPCS(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.dpcs();
  gte.updateError();
}

auto CPU::instructionDPCT(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.dpct();
  gte.updateError();
}

auto CPU::instructionGPF(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.gpf();
  gte.updateError();
}

auto CPU::instructionGPL(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.gpl();
  gte.updateError();
}

auto CPU::instructionINTPL(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.intpl();
  gte.updateError();
}

auto CPU::instructionMVMVA(bool lm, u8 tv, u8 mv, u8 mm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.tv = tv;
  gte.mv = mv;
  gte.mm = mm;
  gte.sf = sf;
  gte.mvmva();
  gte.updateError();
}

auto CPU::instructionNCCS(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.nccs();
  gte.updateError();
}

auto CPU::instructionNCCT(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
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

auto CPU::instructionNCS(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.ncs();
  gte.updateError();
}

auto CPU::instructionNCT(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.nct();
  gte.updateError();
}

auto CPU::instructionOP(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
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

auto CPU::instructionSQR(bool lm, u8 sf) -> void {
  gte.clearFlags();
  gte.lm = lm;
  gte.sf = sf;
  gte.sqr();
  gte.updateError();
}
