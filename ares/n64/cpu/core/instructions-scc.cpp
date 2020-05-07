auto CPU::getControlRegister(uint5 index) -> u64 {
  uint64 data;
  switch(index) {
  case 12:  //status
    data.bit( 0)    = scc.status.interruptEnable;
    data.bit( 1)    = scc.status.exceptionLevel;
    data.bit( 2)    = scc.status.errorLevel;
    data.bit( 3, 4) = scc.status.privilegeMode;
    data.bit( 5)    = scc.status.userMode;
    data.bit( 6)    = scc.status.supervisorMode;
    data.bit( 7)    = scc.status.kernelMode;
    data.bit( 8,15) = scc.status.interruptMask;
    data.bit(16)    = scc.status.de;
    data.bit(17)    = scc.status.ce;
    data.bit(18)    = scc.status.condition;
    data.bit(20)    = scc.status.softReset;
    data.bit(21)    = scc.status.tlbShutdown;
    data.bit(22)    = scc.status.vectorLocation;
    data.bit(24)    = scc.status.instructionTracing;
    data.bit(25)    = scc.status.reverseEndian;
    data.bit(26)    = scc.status.floatingPointMode;
    data.bit(27)    = scc.status.lowPowerMode;
    data.bit(28)    = scc.status.enable.coprocessor0;
    data.bit(29)    = scc.status.enable.coprocessor1;
    data.bit(30)    = scc.status.enable.coprocessor2;
    data.bit(31)    = scc.status.enable.coprocessor3;
    break;
  case 13:  //cause
    data.bit( 2, 6) = scc.cause.exceptionCode;
    data.bit( 8,15) = scc.cause.interruptPending;
    data.bit(28,29) = scc.cause.coprocessorError;
    data.bit(31)    = scc.cause.branchDelay;
    break;
  case 14:  //exception program counter
    data = scc.epc.u64;
    break;
  case 15:  //processor revision identifier
    data = 0x0b00;  //VR4300 product ID
    break;
  case 17:  //load linked address
    data = scc.ll.u64;
    break;
  }
  return data;
}

auto CPU::setControlRegister(uint5 index, uint64 data) -> void {
  switch(index) {
  case 12:  //status
    scc.status.interruptEnable     = data.bit( 0);
    scc.status.exceptionLevel      = data.bit( 1);
    scc.status.errorLevel          = data.bit( 2);
    scc.status.privilegeMode       = data.bit( 3, 4);
    scc.status.userMode            = data.bit( 5);
    scc.status.supervisorMode      = data.bit( 6);
    scc.status.kernelMode          = data.bit( 7);
    scc.status.interruptMask       = data.bit( 8,15);
    scc.status.de                  = data.bit(16);
    scc.status.ce                  = data.bit(17);
    scc.status.condition           = data.bit(18);
    scc.status.softReset           = data.bit(20);
  //scc.status.tlbShutdown         = data.bit(21);  //read-only
    scc.status.vectorLocation      = data.bit(22);
    scc.status.instructionTracing  = data.bit(24);
    scc.status.reverseEndian       = data.bit(25);
    scc.status.floatingPointMode   = data.bit(26);
    scc.status.lowPowerMode        = data.bit(27);
    scc.status.enable.coprocessor0 = data.bit(28);
    scc.status.enable.coprocessor1 = data.bit(29);
    scc.status.enable.coprocessor2 = data.bit(30);
    scc.status.enable.coprocessor3 = data.bit(31);
    break;
  case 13:  //cause
    scc.cause.interruptPending.bit(0) = data.bit(8);
    scc.cause.interruptPending.bit(1) = data.bit(9);
    break;
  case 14:  //exception program counter
    scc.epc.u64 = data;
    break;
  case 17:  //load linked address
    scc.ll.u64 = data;
    break;
  }
}

//

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
