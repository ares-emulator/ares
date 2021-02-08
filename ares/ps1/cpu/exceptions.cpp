//used to determine if a function call (eg CPU::fetch) triggered an exception
auto CPU::Exception::operator()() -> bool {
  return triggered;
}

auto CPU::Exception::trigger(u32 code) -> void {
  triggered = true;
  self.debugger.exception(code);

  self.scc.status.frame[2] = self.scc.status.frame[1];
  self.scc.status.frame[1] = self.scc.status.frame[0];
  self.scc.status.frame[0] = {};

  self.scc.cause.exceptionCode = code;
  self.scc.cause.coprocessorError = self.pipeline.instruction >> 26;
  self.scc.cause.branchDelay = self.delay.branch[0].slot;
  self.scc.cause.branchTaken = self.delay.branch[0].take;

  self.scc.epc = self.ipu.pc;
  if(self.scc.cause.branchDelay) {
    self.scc.epc -= 4;
    if(self.scc.cause.branchTaken) {
      self.scc.targetAddress = self.delay.branch[0].address;
    } else {
      self.scc.targetAddress = self.ipu.pd;
    }
  }

  //exceptions and interrupts discard the execution of delay slots
  self.delay.branch[0] = {};
  self.delay.branch[1] = {};
  self.ipu.pc = !self.scc.status.vectorLocation ? 0x8000'0080 : 0xbfc0'0180;
  self.ipu.pd = self.ipu.pc;
}

auto CPU::Exception::interruptsPending() -> u8 {
  if(!self.scc.status.frame[0].interruptEnable) return 0x00;
  if(self.delay.interrupt && --self.delay.interrupt) return 0x00;
  return self.scc.cause.interruptPending & self.scc.status.interruptMask;
}

auto CPU::Exception::interrupt() -> void {
  trigger(0);
}

template<u32 Mode>
auto CPU::Exception::address(u32 address) -> void {
  if constexpr(Mode == Read ) trigger(4);
  if constexpr(Mode == Write) trigger(5);
  self.scc.badVirtualAddress = address;
}

auto CPU::Exception::busInstruction() -> void {
  trigger(6);
}

auto CPU::Exception::busData() -> void {
  trigger(7);
  self.scc.cause.coprocessorError = 0;
}

auto CPU::Exception::systemCall() -> void {
  trigger(8);
}

auto CPU::Exception::breakpoint(bool overrideVectorLocation) -> void {
  trigger(9);
  //todo: is this really 0xbfc0'0140 when BEV=1?
  if(overrideVectorLocation) self.ipu.pc -= 0x40, self.ipu.pd -= 0x40;
}

auto CPU::Exception::reservedInstruction() -> void {
  trigger(10);
}

auto CPU::Exception::coprocessor() -> void {
  trigger(11);
}

auto CPU::Exception::arithmeticOverflow() -> void {
  trigger(12);
}

auto CPU::Exception::trap() -> void {
  trigger(13);
}
