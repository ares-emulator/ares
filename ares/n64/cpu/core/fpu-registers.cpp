auto CPU::getControlRegisterFPU(uint5 index) -> u32 {
  uint32 data;
  switch(index) {
  case  0:  //coprocessor revision identifier
    data.bit(0, 7) = fpu.coprocessor.revision;
    data.bit(8,15) = fpu.coprocessor.implementation;
    break;
  case 31:  //control / status register
    data.bit( 0) = fpu.csr.roundMode.bit(0);
    data.bit( 1) = fpu.csr.roundMode.bit(1);
    data.bit( 2) = fpu.csr.flag.inexact;
    data.bit( 3) = fpu.csr.flag.underflow;
    data.bit( 4) = fpu.csr.flag.overflow;
    data.bit( 5) = fpu.csr.flag.divisionByZero;
    data.bit( 6) = fpu.csr.flag.invalidOperation;
    data.bit( 7) = fpu.csr.enable.inexact;
    data.bit( 8) = fpu.csr.enable.underflow;
    data.bit( 9) = fpu.csr.enable.overflow;
    data.bit(10) = fpu.csr.enable.divisionByZero;
    data.bit(11) = fpu.csr.enable.invalidOperation;
    data.bit(12) = fpu.csr.cause.inexact;
    data.bit(13) = fpu.csr.cause.underflow;
    data.bit(14) = fpu.csr.cause.overflow;
    data.bit(15) = fpu.csr.cause.divisionByZero;
    data.bit(16) = fpu.csr.cause.invalidOperation;
    data.bit(17) = fpu.csr.cause.unimplementedOperation;
    data.bit(23) = fpu.csr.compare;
    data.bit(24) = fpu.csr.flushed;
    break;
  }
  return data;
}

auto CPU::setControlRegisterFPU(uint5 index, uint32 data) -> void {
  //read-only variables are defined but commented out for documentation purposes
  switch(index) {
  case  0:  //coprocessor revision identifier
  //fpu.coprocessor.revision       = data.bit(0, 7);
  //fpu.coprocessor.implementation = data.bit(8,15);
    break;
  case 31: {//control / status register
    uint roundModePrevious = fpu.csr.roundMode;
    fpu.csr.roundMode.bit(0)             = data.bit( 0);
    fpu.csr.roundMode.bit(1)             = data.bit( 1);
    fpu.csr.flag.inexact                 = data.bit( 2);
    fpu.csr.flag.underflow               = data.bit( 3);
    fpu.csr.flag.overflow                = data.bit( 4);
    fpu.csr.flag.divisionByZero          = data.bit( 5);
    fpu.csr.flag.invalidOperation        = data.bit( 6);
    fpu.csr.enable.inexact               = data.bit( 7);
    fpu.csr.enable.underflow             = data.bit( 8);
    fpu.csr.enable.overflow              = data.bit( 9);
    fpu.csr.enable.divisionByZero        = data.bit(10);
    fpu.csr.enable.invalidOperation      = data.bit(11);
    fpu.csr.cause.inexact                = data.bit(12);
    fpu.csr.cause.underflow              = data.bit(13);
    fpu.csr.cause.overflow               = data.bit(14);
    fpu.csr.cause.divisionByZero         = data.bit(15);
    fpu.csr.cause.invalidOperation       = data.bit(16);
    fpu.csr.cause.unimplementedOperation = data.bit(17);
    fpu.csr.compare                      = data.bit(23);
    fpu.csr.flushed                      = data.bit(24);

    if(fpu.csr.roundMode != roundModePrevious) {
      switch(fpu.csr.roundMode) {
      case 0: fesetround(FE_TONEAREST);  break;
      case 1: fesetround(FE_TOWARDZERO); break;
      case 2: fesetround(FE_UPWARD);     break;
      case 3: fesetround(FE_DOWNWARD);   break;
      }
    }
  } break;
  }
}
