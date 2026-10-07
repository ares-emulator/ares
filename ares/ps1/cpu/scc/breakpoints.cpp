auto CPU::testCodeBreakpoint(u32 address) -> bool {
  scc.breakpoint.lastPC = ipu.pc;

  if(!scc.breakpoint.enable.master) return false;
  if(!scc.breakpoint.enable.kernel && (address >> 31) == 1) return false;
  if(!scc.breakpoint.enable.user   && (address >> 31) == 0) return false;
  if(!scc.breakpoint.test.code) return false;
  if((address ^ scc.breakpoint.address.code) & scc.breakpoint.mask.code) return false;

  bool triggered = false;

  if(scc.breakpoint.test.code) {
    scc.breakpoint.status.any = 1;
    scc.breakpoint.status.code = 1;
    triggered = true;
  }

  if(scc.breakpoint.test.trace && ipu.pc != scc.breakpoint.lastPC + 4) {
    scc.breakpoint.status.any = 1;
    scc.breakpoint.status.trace = 1;
    triggered = true;
  }

  if(!triggered) return false;
  if(!scc.breakpoint.enable.trap) return false;

  if constexpr(Accuracy::CPU::AddressErrors) {
    if(address & 3) return exception.address<Read>(address), true;
  }
  exception.breakpoint(1);
  return true;
}

template<u32 Mode, u32 Size>
auto CPU::testDataBreakpoint(u32 address) -> bool {
  if(!scc.breakpoint.enable.master) return false;
  if(!scc.breakpoint.enable.kernel && (address >> 31) == 1) return false;
  if(!scc.breakpoint.enable.user   && (address >> 31) == 0) return false;
  if(!scc.breakpoint.test.data) return false;
  if constexpr(Mode == Read ) if(!scc.breakpoint.test.read ) return false;
  if constexpr(Mode == Write) if(!scc.breakpoint.test.write) return false;
  if((address ^ scc.breakpoint.address.data) & scc.breakpoint.mask.data) return false;

  scc.breakpoint.status.any = 1;
  scc.breakpoint.status.data = 1;
  if constexpr(Mode == Read ) scc.breakpoint.status.read  = 1;
  if constexpr(Mode == Write) scc.breakpoint.status.write = 1;
  if(!scc.breakpoint.enable.trap) return false;

  if constexpr(Accuracy::CPU::AddressErrors) {
    if constexpr(Size == Half) {
      if(address & 1) return exception.address<Mode>(address), true;
    }
    if constexpr(Size == Word) {
      if(address & 3) return exception.address<Mode>(address), true;
    }
  }
  exception.breakpoint(1);
  return true;
}
