auto CPU::Breakpoint::testCode(u32 address) -> bool {
  lastPC = self.ipu.pc;

  if(!self.scc.breakpoint.enable.master) return false;
  if(!self.scc.breakpoint.enable.kernel && (address >> 31) == 1) return false;
  if(!self.scc.breakpoint.enable.user   && (address >> 31) == 0) return false;
  if(!self.scc.breakpoint.test.code) return false;
  if((address ^ self.scc.breakpoint.address.code) & self.scc.breakpoint.mask.code) return false;

  bool triggered = false;

  if(self.scc.breakpoint.test.code) {
    self.scc.breakpoint.status.any = 1;
    self.scc.breakpoint.status.code = 1;
    triggered = true;
  }

  if(self.scc.breakpoint.test.trace && self.ipu.pc != lastPC + 4) {
    self.scc.breakpoint.status.any = 1;
    self.scc.breakpoint.status.trace = 1;
    triggered = true;
  }

  if(!triggered) return false;
  if(!self.scc.breakpoint.enable.trap) return false;

  if constexpr(Accuracy::CPU::AddressErrors) {
    if(address & 3) return self.exception.address<Read>(address), true;
  }
  self.exception.breakpoint(1);
  return true;
}

template<u32 Mode, u32 Size>
auto CPU::Breakpoint::testData(u32 address) -> bool {
  if(!self.scc.breakpoint.enable.master) return false;
  if(!self.scc.breakpoint.enable.kernel && (address >> 31) == 1) return false;
  if(!self.scc.breakpoint.enable.user   && (address >> 31) == 0) return false;
  if(!self.scc.breakpoint.test.data) return false;
  if constexpr(Mode == Read ) if(!self.scc.breakpoint.test.read ) return false;
  if constexpr(Mode == Write) if(!self.scc.breakpoint.test.write) return false;
  if((address ^ self.scc.breakpoint.address.data) & self.scc.breakpoint.mask.data) return false;

  self.scc.breakpoint.status.any = 1;
  self.scc.breakpoint.status.data = 1;
  if constexpr(Mode == Read ) self.scc.breakpoint.status.read  = 1;
  if constexpr(Mode == Write) self.scc.breakpoint.status.write = 1;
  if(!self.scc.breakpoint.enable.trap) return false;

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(address & 1) return self.exception.address<Mode>(address), true;
    }
    if constexpr(Size == Word) {
      if(address & 3) return self.exception.address<Mode>(address), true;
    }
  }
  self.exception.breakpoint(1);
  self.scc.cause.coprocessorError = 0;
  return true;
}
