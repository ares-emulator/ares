auto CPU::getControlRegister(u8 index) -> u32 {
  uint32 data = 0;

  switch(index & 15) {

  case  3:  //Breakpoint Execute Address
    data.bit(0,31) = scc.breakpointExecuteAddress;
    break;

  case  5:  //Breakpoint Data Address
    data.bit(0,31) = scc.breakpointDataAddress;
    break;

  case  7:  //Breakpoint Control
  //todo
    break;

  case  8:  //Bad Virtual Address
    data.bit(0,31) = scc.badVirtualAddress;
    break;

  case  9:  //Breakpoint Data Mask
    data.bit(0,31) = scc.breakpointDataMask;
    break;

  case 11:  //Breakpoint Execute Mask
    data.bit(0,31) = scc.breakpointExecuteMask;
    break;

  case 12:  //Status
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
    break;

  case 13:  //Cause
    data.bit( 2, 6) = scc.cause.exceptionCode;
    data.bit( 8,15) = scc.cause.interruptPending;
    data.bit(28,29) = scc.cause.coprocessorError;
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

auto CPU::setControlRegister(u8 index, u32 value) -> void {
  uint32 data = value;

  switch(index & 15) {

  case  3:  //Breakpoint Execute Address
    scc.breakpointExecuteAddress = data;
    break;

  case  5:  //Breakpoint Data Address
    scc.breakpointDataAddress = data;
    break;

  case  7:  //Breakpoint Control
  //todo
    break;

  case  8:  //Bad Virtual Address
  //scc.badVirtualAddress = data;  //read-only
    break;

  case  9:  //Breakpoint Data Mask
    scc.breakpointDataMask = data;
    break;

  case 11:  //Breakpoint Execute Mask
    scc.breakpointExecuteMask = data;
    break;

  case 12:  //Status
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
    break;

  case 13:  //Cause
    scc.cause.interruptPending.bit(0) = data.bit(8);
    scc.cause.interruptPending.bit(1) = data.bit(9);
    break;

  case 14:  //Exception Program Counter
    scc.epc = data;
    break;

  case 15:  //Product ID
  //scc.productID.revision       = data.bit(0, 7);  //read-only
  //scc.productID.implementation = data.bit(8,15);  //read-only
    break;

  }
}
