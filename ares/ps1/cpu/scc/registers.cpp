auto CPU::statusRegisterSCC() const -> u32 {
  n32 data = 0;
  data.bit( 0)    = scc.status.frame[0].interruptEnable;
  data.bit( 1)    = scc.status.frame[0].userMode;
  data.bit( 2)    = scc.status.frame[1].interruptEnable;
  data.bit( 3)    = scc.status.frame[1].userMode;
  data.bit( 4)    = scc.status.frame[2].interruptEnable;
  data.bit( 5)    = scc.status.frame[2].userMode;
  data.bit( 8,15) = scc.status.interruptMask;
  data.bit(16)    = scc.status.cache.isolate;
  data.bit(17)    = scc.status.cache.swap;
  data.bit(18)    = scc.status.cache.parityZero;
  data.bit(19)    = scc.status.cache.loadWasData;
  data.bit(20)    = scc.status.cache.parityError;
  data.bit(21)    = scc.status.tlbShutdown;
  data.bit(22)    = scc.status.vectorLocation;
  data.bit(25)    = scc.status.reverseEndian;
  data.bit(28)    = scc.status.enable.coprocessor0;
  data.bit(29)    = scc.status.enable.coprocessor1;
  data.bit(30)    = scc.status.enable.coprocessor2;
  data.bit(31)    = scc.status.enable.coprocessor3;
  return data;
}

auto CPU::setStatusRegisterSCC(u32 value) -> void {
  n32 data = value;
  scc.status.frame[0].interruptEnable = data.bit( 0);
  scc.status.frame[0].userMode        = data.bit( 1);
  scc.status.frame[1].interruptEnable = data.bit( 2);
  scc.status.frame[1].userMode        = data.bit( 3);
  scc.status.frame[2].interruptEnable = data.bit( 4);
  scc.status.frame[2].userMode        = data.bit( 5);
  scc.status.interruptMask            = data.bit( 8,15);
  scc.status.cache.isolate            = data.bit(16);
  scc.status.cache.swap               = data.bit(17);
  scc.status.cache.parityZero         = data.bit(18);
  scc.status.cache.loadWasData        = data.bit(19);
  scc.status.cache.parityError        = data.bit(20);
//scc.status.tlbShutdown              = data.bit(21);  //read-only
  scc.status.vectorLocation           = data.bit(22);
  scc.status.reverseEndian            = data.bit(25);
  scc.status.enable.coprocessor0      = data.bit(28);
  scc.status.enable.coprocessor1      = data.bit(29);
  scc.status.enable.coprocessor2      = data.bit(30);
  scc.status.enable.coprocessor3      = data.bit(31);
}

auto CPU::effectiveStatusRegisterSCC() const -> u32 {
  if(execution.status.managed) return execution.status.visible;
  return statusRegisterSCC();
}

auto CPU::statusInterruptEnable() const -> bool {
  return effectiveStatusRegisterSCC() & 1;
}

auto CPU::statusUserMode() const -> bool {
  return effectiveStatusRegisterSCC() >> 1 & 1;
}

auto CPU::statusCacheIsolated() const -> bool {
  return effectiveStatusRegisterSCC() >> 16 & 1;
}

auto CPU::statusVectorLocation() const -> bool {
  return effectiveStatusRegisterSCC() >> 22 & 1;
}

auto CPU::statusInterruptMask() const -> u8 {
  return effectiveStatusRegisterSCC() >> 8;
}

auto CPU::statusCoprocessorEnabled(u32 coprocessor) const -> bool {
  return effectiveStatusRegisterSCC() >> (28 + coprocessor) & 1;
}

auto CPU::getControlRegisterSCC(u8 index) -> u32 {
  n32 data = 0;

  switch(index & 15) {

  case  3:  //Breakpoint Code Address
    data.bit(0,31) = scc.breakpoint.address.code;
    break;

  case  5:  //Breakpoint Data Address
    data.bit(0,31) = scc.breakpoint.address.data;
    break;

  case  6:  //Target Address
    data.bit(0,31) = scc.targetAddress;
    break;

  case  7:  //Breakpoint Control
    data.bit( 0) = scc.breakpoint.status.any;
    data.bit( 1) = scc.breakpoint.status.code;
    data.bit( 2) = scc.breakpoint.status.data;
    data.bit( 3) = scc.breakpoint.status.read;
    data.bit( 4) = scc.breakpoint.status.write;
    data.bit( 5) = scc.breakpoint.status.trace;
    data.bit(12) = scc.breakpoint.redirection.bit(0);
    data.bit(13) = scc.breakpoint.redirection.bit(1);
    data.bit(14) = scc.breakpoint.unknown.bit(0);
    data.bit(15) = scc.breakpoint.unknown.bit(1);
    data.bit(23) = scc.breakpoint.enable.master;
    data.bit(24) = scc.breakpoint.test.code;
    data.bit(25) = scc.breakpoint.test.data;
    data.bit(26) = scc.breakpoint.test.read;
    data.bit(27) = scc.breakpoint.test.write;
    data.bit(28) = scc.breakpoint.test.trace;
    data.bit(29) = scc.breakpoint.enable.kernel;
    data.bit(30) = scc.breakpoint.enable.user;
    data.bit(31) = scc.breakpoint.enable.trap;
    break;

  case  8:  //Bad Virtual Address
    data.bit(0,31) = scc.badVirtualAddress;
    break;

  case  9:  //Breakpoint Data Mask
    data.bit(0,31) = scc.breakpoint.mask.data;
    break;

  case 11:  //Breakpoint Code Mask
    data.bit(0,31) = scc.breakpoint.mask.code;
    break;

  case 12:  //Status
    data = statusRegisterSCC();
    break;

  case 13:  //Cause
    data.bit( 2, 6) = scc.cause.exceptionCode;
    data.bit( 8,15) = scc.cause.interruptPending;
    data.bit(28,29) = scc.cause.coprocessorError;
    data.bit(30)    = scc.cause.branchTaken;
    data.bit(31)    = scc.cause.branchDelay;
    break;

  case 14:
    data.bit(0,31) = scc.epc;
    break;

  case 15:  //Product ID
    data.bit(0, 7) = scc.productID.revision;
    data.bit(8,15) = scc.productID.implementation;
    break;

  }

  return data;
}

auto CPU::setControlRegisterSCC(u8 index, u32 value) -> void {
  n32 data = value;

  switch(index & 15) {

  case  3:  //Breakpoint Code Address
    scc.breakpoint.address.code = data;
    break;

  case  5:  //Breakpoint Data Address
    scc.breakpoint.address.data = data;
    break;

  case  6:  //Target Address
  //scc.targetAddress = data;  //read-only
    break;

  case  7:  //Breakpoint Control
    scc.breakpoint.status.any           = data.bit( 0);
    scc.breakpoint.status.code          = data.bit( 1);
    scc.breakpoint.status.data          = data.bit( 2);
    scc.breakpoint.status.read          = data.bit( 3);
    scc.breakpoint.status.write         = data.bit( 4);
    scc.breakpoint.status.trace         = data.bit( 5);
    scc.breakpoint.redirection.bit(0)   = data.bit(12);
    scc.breakpoint.redirection.bit(1)   = data.bit(13);
    scc.breakpoint.unknown.bit(0)       = data.bit(14);
    scc.breakpoint.unknown.bit(1)       = data.bit(15);
    scc.breakpoint.enable.master        = data.bit(23);
    scc.breakpoint.test.code            = data.bit(24);
    scc.breakpoint.test.data            = data.bit(25);
    scc.breakpoint.test.read            = data.bit(26);
    scc.breakpoint.test.write           = data.bit(27);
    scc.breakpoint.test.trace           = data.bit(28);
    scc.breakpoint.enable.kernel        = data.bit(29);
    scc.breakpoint.enable.user          = data.bit(30);
    scc.breakpoint.enable.trap          = data.bit(31);
    break;

  case  8:  //Bad Virtual Address
  //scc.badVirtualAddress = data;  //read-only
    break;

  case  9:  //Breakpoint Data Mask
    scc.breakpoint.mask.data = data;
    break;

  case 11:  //Breakpoint Code Mask
    scc.breakpoint.mask.code = data;
    break;

  case 12: {//Status
    u32 previous = effectiveStatusRegisterSCC();
    setStatusRegisterSCC(data);
    scheduleStatusVisibility(previous, statusRegisterSCC(), execution.retiredInstructions + 3);
    break;
  }

  case 13: {//Cause
    scc.cause.interruptPending.bit(0) = data.bit(8);
    scc.cause.interruptPending.bit(1) = data.bit(9);
    break;
  }

  case 14:  //Exception Program Counter
    scc.epc = data;
    break;

  case 15:  //Product ID
  //scc.productID.revision       = data.bit(0, 7);  //read-only
  //scc.productID.implementation = data.bit(8,15);  //read-only
    break;

  }
}
