auto CPU::instructionBC0() -> void {
  bool condition = OP >> 16 & 1;
  bool likely    = OP >> 17 & 1;
  if(scc.cf == condition) IP = PC + (IMMi16 << 2);
  else if(likely) PC += 4;
}

auto CPU::instructionCFC0() -> void {
  RT.u64 = i32(scc.cr[RDn]);
}

auto CPU::instructionCTC0() -> void {
  scc.cr[RDn] = RT.u32;
}

auto CPU::instructionDMFC0() -> void {
  RT.u64 = getControlRegister(RDn);
}

auto CPU::instructionDMTC0() -> void {
  setControlRegister(RDn, RT.u64);
}

auto CPU::instructionERET() -> void {
  IP = scc.epc.u64;
  if(scc.status.errorLevel) {
    scc.status.errorLevel = 0;
  } else {
    scc.status.exceptionLevel = 0;
  }
  scc.llbit = 0;
}

auto CPU::instructionMFC0() -> void {
  RT.u64 = i32(getControlRegister(RDn));
}

auto CPU::instructionMTC0() -> void {
  setControlRegister(RDn, RT.u32);
}

auto CPU::instructionTLBP() -> void {
  //todo
}

auto CPU::instructionTLBR() -> void {
  //todo
}

auto CPU::instructionTLBWI() -> void {
  //todo
}

auto CPU::instructionTLBWR() -> void {
  //todo
}
