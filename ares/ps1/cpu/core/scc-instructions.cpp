auto CPU::instructionMFC0(u32& rt, u8 rd) -> void {
  fetch(rt, getControlRegister(rd));
}

auto CPU::instructionMTC0(cu32& rt, u8 rd) -> void {
  setControlRegister(rd, rt);
}

auto CPU::instructionRFE() -> void {
  scc.status.frame[0] = scc.status.frame[1];
  scc.status.frame[1] = scc.status.frame[2];
  scc.status.frame[2] = {};
}
