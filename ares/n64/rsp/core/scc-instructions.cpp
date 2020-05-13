auto RSP::instructionMFC0() -> void {
  RT.u32 = i32(getControlRegister(RDn));
}

auto RSP::instructionMTC0() -> void {
  setControlRegister(RDn, RT.u32);
}
