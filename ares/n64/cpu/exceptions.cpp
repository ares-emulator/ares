auto CPU::Exception::trigger(u32 code, u32 coprocessor) -> void {
  if(self.debugger.tracer.exception->enabled()) {
    if(code != 0) self.debugger.exception(hex(code, 2L));
  }

  if(!self.scc.status.exceptionLevel) {
    self.scc.epc = self.ipu.pc;
    self.scc.status.exceptionLevel = 1;
    self.scc.cause.exceptionCode = code;
    self.scc.cause.coprocessorError = coprocessor;
    if(self.scc.cause.branchDelay = self.branch.inDelaySlot()) self.scc.epc -= 4;
  } else {
    self.scc.cause.exceptionCode = code;
    self.scc.cause.coprocessorError = coprocessor;
  }

  u64 vectorBase = !self.scc.status.vectorLocation ? s32(0x8000'0000) : s32(0xbfc0'0200);
  u32 vectorOffset = (code == 2 || code == 3) ? 0x0000 : 0x0180;
  self.ipu.pc = vectorBase + vectorOffset;
  self.branch.exception();
  self.context.setMode();
}

auto CPU::Exception::interrupt() -> void { trigger(0); }
auto CPU::Exception::tlbModification() -> void { trigger(1); }
auto CPU::Exception::tlbLoad() -> void { trigger(2); }
auto CPU::Exception::tlbStore() -> void { trigger(3); }
auto CPU::Exception::addressLoad() -> void { trigger(4); }
auto CPU::Exception::addressStore() -> void { trigger(5); }
auto CPU::Exception::busInstruction() -> void { trigger(6); }
auto CPU::Exception::busData() -> void { trigger(7); }
auto CPU::Exception::systemCall() -> void { trigger(8); }
auto CPU::Exception::breakpoint() -> void { trigger(9); }
auto CPU::Exception::reservedInstruction() -> void { trigger(10); }
auto CPU::Exception::coprocessor0() -> void { trigger(11, 0); }
auto CPU::Exception::coprocessor1() -> void { trigger(11, 1); }
auto CPU::Exception::coprocessor2() -> void { trigger(11, 2); }
auto CPU::Exception::coprocessor3() -> void { trigger(11, 3); }
auto CPU::Exception::arithmeticOverflow() -> void { trigger(12); }
auto CPU::Exception::trap() -> void { trigger(13); }
auto CPU::Exception::floatingPoint() -> void { trigger(15); }
auto CPU::Exception::watchAddress() -> void { trigger(23); }
