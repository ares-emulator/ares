auto CPU::getCOP0u64(uint index) -> u64 {
  auto data = SPR[index].u64;
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
  default:
    SPR[index].u64 = data;
    break;
  }
}

//

auto CPU::instructionBC0F() -> void {
  if(cop0.cf == 0) IP = PC + (IMMi16 << 2);
}

auto CPU::instructionBC0FL() -> void {
  if(cop0.cf == 0) IP = PC + (IMMi16 << 2);
  else PC += 4;
}

auto CPU::instructionBC0T() -> void {
  if(cop0.cf == 1) IP = PC + (IMMi16 << 2);
}

auto CPU::instructionBC0TL() -> void {
  if(cop0.cf == 1) IP = PC + (IMMi16 << 2);
  else PC += 4;
}

auto CPU::instructionCFC0() -> void {
  RT.u64 = i32(cop0.cr[RDn]);
}

auto CPU::instructionCTC0() -> void {
  cop0.cr[RDn] = RT.u32;
}

auto CPU::instructionDMFC0() -> void {
  RT.u64 = getCOP0u64(RDn);
}

auto CPU::instructionDMTC0() -> void {
  setCOP0u64(RDn, RT.u64);
}

auto CPU::instructionMFC0() -> void {
  RT.u64 = i32(getCOP0u64(RDn));
}

auto CPU::instructionMTC0() -> void {
  setCOP0u64(RDn, RT.u32);
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
