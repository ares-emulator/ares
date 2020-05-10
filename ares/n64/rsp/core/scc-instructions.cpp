auto RSP::instructionCFC0() -> void {
  //todo
}

auto RSP::instructionCTC0() -> void {
  //todo
}

auto RSP::instructionMFC0() -> void {
  RT.u64 = i32(getControlRegister(RDn));
}

auto RSP::instructionMTC0() -> void {
  setControlRegister(RDn, RT.u32);
}
