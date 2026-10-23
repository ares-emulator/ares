auto CPU::instruction() -> void {
  if constexpr(Accuracy::CPU::Breakpoints) {
    if(unlikely(testCodeBreakpoint(ipu.pc))) {
        return (void)instructionEpilogue();
    }
  }

  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(ipu.pc & 3)) {
      exception.address<Read>(ipu.pc);
        return (void)instructionEpilogue();
    }
  }

  u32 instruction = fetch(ipu.pc);
  if(exception()) return (void)instructionEpilogue();

  instructionPrologue(instruction);
  decoderEXECUTE();
  instructionEpilogue();
}

auto CPU::instructionPrologue(u32 instruction) -> void {
  pipeline.address = ipu.pc;
  pipeline.instruction = instruction;
  debugger.instruction();
}

auto CPU::instructionEpilogue() -> void {
  ipu.pb = ipu.pc;
  ipu.pc = ipu.pd;
  ipu.pd = ipu.pd + 4;

  processDelayLoad();
  processDelayBranch();
  ipu.r[0] = 0;  //it's faster to allow assigning to r0 and then clearing it later

  execution.retiredInstructions++;
  retireStatusVisibility();
  if(auto interrupts = exception.interruptsPending()) {
    debugger.interrupt(scc.cause.interruptPending);
    exception.interrupt();
  }
  exception.triggered = 0;

  //When a branch is detected, check if we need to hook a  bios call
  if(ipu.pb + 4 != ipu.pc) {
    debugger.branch();
  }
}
