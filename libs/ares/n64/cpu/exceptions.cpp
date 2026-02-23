auto CPU::Exception::trigger(u32 code, u32 coprocessor, bool tlbMiss) -> void {
  self.debugger.exception(code);
  if (code != 0) {  //ignore interrupt exceptions
    reportGDBException(code, self.ipu.pc); 
  }

  u64 vectorBase = !self.scc.status.vectorLocation ? (s32)0x8000'0000 : (s32)0xbfc0'0200;

  u16 vectorOffset = 0x0180;
  if(tlbMiss) {
    //use special vector offset for TLB load/store miss exceptions when EXL=0
    if(!self.scc.status.exceptionLevel) {
      if(self.context.bits == 32) vectorOffset = 0x0000;
      if(self.context.bits == 64) vectorOffset = 0x0080;
    }
  }

  if(!self.scc.status.exceptionLevel) {
    self.scc.epc = self.ipu.pc;
    self.scc.status.exceptionLevel = 1;
    self.scc.cause.exceptionCode = code;
    self.scc.cause.coprocessorError = coprocessor;
    if(self.scc.cause.branchDelay = self.pipeline.inDelaySlot()) self.scc.epc -= 4;
  } else {
    self.scc.cause.exceptionCode = code;
    self.scc.cause.coprocessorError = coprocessor;
  }

  self.pipeline.setPc(vectorBase + vectorOffset);
  self.pipeline.exception();  //todo: only call this between instruction prologue/epilogue
  self.context.setMode();
}

auto CPU::Exception::interrupt()               -> void { trigger( 0); }
auto CPU::Exception::tlbModification()         -> void { trigger( 1); }
auto CPU::Exception::tlbLoadInvalid()          -> void { trigger( 2, 0, 0); }
auto CPU::Exception::tlbLoadMiss()             -> void { trigger( 2, 0, 1); }
auto CPU::Exception::tlbStoreInvalid()         -> void { trigger( 3, 0, 0); }
auto CPU::Exception::tlbStoreMiss()            -> void { trigger( 3, 0, 1); }
auto CPU::Exception::addressLoad()             -> void { trigger( 4); }
auto CPU::Exception::addressStore()            -> void { trigger( 5); }
auto CPU::Exception::busInstruction()          -> void { trigger( 6); }
auto CPU::Exception::busData()                 -> void { trigger( 7); }
auto CPU::Exception::systemCall()              -> void { trigger( 8); }
auto CPU::Exception::breakpoint()              -> void { trigger( 9); }
auto CPU::Exception::reservedInstruction()     -> void { trigger(10); }
auto CPU::Exception::reservedInstructionCop2() -> void { trigger(10, 2); }
auto CPU::Exception::coprocessor0()            -> void { trigger(11, 0); }
auto CPU::Exception::coprocessor1()            -> void { trigger(11, 1); }
auto CPU::Exception::coprocessor2()            -> void { trigger(11, 2); }
auto CPU::Exception::coprocessor3()            -> void { trigger(11, 3); }
auto CPU::Exception::arithmeticOverflow()      -> void { trigger(12); }
auto CPU::Exception::trap()                    -> void { trigger(13); }
auto CPU::Exception::floatingPoint()           -> void { trigger(15); }
auto CPU::Exception::watchAddress()            -> void { trigger(23); }

auto CPU::Exception::nmi() -> void {
  self.scc.status.vectorLocation = 1;
  self.scc.status.tlbShutdown = 0;
  self.scc.status.softReset = 0;
  self.scc.status.errorLevel = 1;
  self.scc.epcError = self.ipu.pc;
  self.pipeline.setPc(0xffff'ffff'bfc0'0000);
}

auto CPU::Exception::reportGDBException(int code, u64 pc) -> void {
  switch(code) {
  case 0:  GDB::server.reportSignal(GDB::Signal::WINCH, pc); break; // ignored by default
  case 1:  GDB::server.reportSignal(GDB::Signal::SEGV,  pc); break;
  case 2:  GDB::server.reportSignal(GDB::Signal::SEGV,  pc); break;
  case 3:  GDB::server.reportSignal(GDB::Signal::SEGV,  pc); break;
  case 4:  GDB::server.reportSignal(GDB::Signal::BUS,   pc); break;
  case 5:  GDB::server.reportSignal(GDB::Signal::BUS,   pc); break;
  case 6:  GDB::server.reportSignal(GDB::Signal::ABORT, pc); break;
  case 7:  GDB::server.reportSignal(GDB::Signal::ABORT, pc); break;
  case 8:  GDB::server.reportSignal(GDB::Signal::SYS,   pc); break;
  case 9:  GDB::server.reportSignal(GDB::Signal::STOP,  pc); break;
  case 10: GDB::server.reportSignal(GDB::Signal::ILL,   pc); break;
  case 11: GDB::server.reportSignal(GDB::Signal::URG,   pc); break; // ignored by default
  case 12: GDB::server.reportSignal(GDB::Signal::FPE,   pc); break;
  case 13: GDB::server.reportSignal(GDB::Signal::TRAP,  pc); break;
  case 15: GDB::server.reportSignal(GDB::Signal::FPE,   pc); break;
  case 23: GDB::server.reportSignal(GDB::Signal::LOST,  pc); break;
  }
}
