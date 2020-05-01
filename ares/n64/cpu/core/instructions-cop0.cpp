auto CPU::getCOP0u64(uint index) -> u64 {
  u64 data = cop0.r[index];
  switch(index) {
  case COP0::Register::PRID:
    data = 0x0b00;  //VR4300 product ID
    break;
  }
  return data;
}

auto CPU::setCOP0u64(uint index, u64 data) -> void {
  switch(index) {
  case COP0::Register::Cause:
    CAUSE = CAUSE & 0xfc00 | data & ~0xfc00;
    if(CAUSE & 0x0300) {
      if(IP) {
        PC = IP();
        IP = nothing;
      }
      exception(Interrupt);
    }
    break;
  case COP0::Register::PRID:
    break;
  }
}

//

auto CPU::instructionBC0F() -> void {
  if(cop0.cf == 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBC0FL() -> void {
  if(cop0.cf == 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBC0T() -> void {
  if(cop0.cf == 1) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBC0TL() -> void {
  if(cop0.cf == 1) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionCFC0() -> void {
  wRT64u(i32(cop0.cr[RDn]));
}

auto CPU::instructionCTC0() -> void {
  cop0.cr[RDn] = RT32u;
}

auto CPU::instructionDMFC0() -> void {
  wRT64u(getCOP0u64(RDn));
}

auto CPU::instructionDMTC0() -> void {
  setCOP0u64(RDn, RT64u);
}

auto CPU::instructionMFC0() -> void {
  wRT32i(getCOP0u64(RDn));
}

auto CPU::instructionMTC0() -> void {
  setCOP0u64(RDn, RT32u);
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
