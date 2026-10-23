auto CPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);
  s(scratchpad);

  s(exeLoaded);
  s(accruedCycles);
  s(cyclesUntilForcedSync);
  s(execution.clock);
  s(execution.multiplyDivide.active);
  s(execution.multiplyDivide.completion);
  s(execution.multiplyDivide.hi);
  s(execution.multiplyDivide.lo);
  s(execution.gteCompletion);
  s(execution.nextEventSequence);
  for(auto& event : execution.events) {
    s(event.type);
    s(event.index);
    s(event.value);
    s(event.completion);
    s(event.sequence);
  }
  s(execution.retiredInstructions);
  s(execution.status.managed);
  s(execution.status.visible);
  s(execution.status.nextSequence);
  for(auto& event : execution.status.events) {
    s(event.active);
    s(event.value);
    s(event.retirement);
    s(event.sequence);
  }
  s(frontend.refill.active);
  s(frontend.refill.address);
  s(frontend.refill.tag);
  s(frontend.refill.line);
  s(frontend.refill.requestedWord);
  s(frontend.refill.nextWord);
  s(frontend.refill.finalWord);
  s(frontend.refill.completedWords);
  s(frontend.refill.started);
  s(frontend.refill.completion);
  s(frontend.refill.granted);
  for(auto& entry : frontend.writeBuffer.entries) {
    s(entry.address);
    s(entry.offset);
    s(entry.data);
    s(entry.byteEnable);
    s(entry.sequence);
  }
  s(frontend.writeBuffer.head);
  s(frontend.writeBuffer.count);
  s(frontend.writeBuffer.completion);
  s(frontend.writeBuffer.nextSequence);
  s(frontend.writeBuffer.ready);
  s(frontend.writeBuffer.delay);
  s(frontend.busWait);

  s(pipeline.address);
  s(pipeline.instruction);

  for(auto& load : delay.load) {
    u32 index = load.target ? load.target - ipu.r : ~0;
    s(index);
    load.target = index < 32 ? &ipu.r[index] : nullptr;
    s(load.source);
  }
  s(delay.branch[0].slot);
  s(delay.branch[0].take);
  s(delay.branch[0].address);
  s(delay.branch[1].slot);
  s(delay.branch[1].take);
  s(delay.branch[1].address);

  for(auto& line : icache.lines) {
    s(line.words);
    s(line.tag);
    s(line.valid);
  }

  s(exception.triggered);

  s(scc.breakpoint.lastPC);

  for(auto& r : ipu.r) s(r);
  s(ipu.lo);
  s(ipu.hi);
  s(ipu.pb);
  s(ipu.pc);
  s(ipu.pd);

  s(scc.breakpoint.address.code);
  s(scc.breakpoint.address.data);
  s(scc.breakpoint.mask.code);
  s(scc.breakpoint.mask.data);
  s(scc.breakpoint.status.any);
  s(scc.breakpoint.status.code);
  s(scc.breakpoint.status.data);
  s(scc.breakpoint.status.read);
  s(scc.breakpoint.status.write);
  s(scc.breakpoint.status.trace);
  s(scc.breakpoint.redirection);
  s(scc.breakpoint.unknown);
  s(scc.breakpoint.test.code);
  s(scc.breakpoint.test.data);
  s(scc.breakpoint.test.read);
  s(scc.breakpoint.test.write);
  s(scc.breakpoint.test.trace);
  s(scc.breakpoint.enable.master);
  s(scc.breakpoint.enable.kernel);
  s(scc.breakpoint.enable.user);
  s(scc.breakpoint.enable.trap);
  s(scc.targetAddress);
  s(scc.badVirtualAddress);
  s(scc.status.frame[0].interruptEnable);
  s(scc.status.frame[0].userMode);
  s(scc.status.frame[1].interruptEnable);
  s(scc.status.frame[1].userMode);
  s(scc.status.frame[2].interruptEnable);
  s(scc.status.frame[2].userMode);
  s(scc.status.interruptMask);
  s(scc.status.cache.isolate);
  s(scc.status.cache.swap);
  s(scc.status.cache.parityZero);
  s(scc.status.cache.loadWasData);
  s(scc.status.cache.parityError);
  s(scc.status.tlbShutdown);
  s(scc.status.vectorLocation);
  s(scc.status.reverseEndian);
  s(scc.status.enable.coprocessor0);
  s(scc.status.enable.coprocessor1);
  s(scc.status.enable.coprocessor2);
  s(scc.status.enable.coprocessor3);
  s(scc.cause.exceptionCode);
  s(scc.cause.interruptPending);
  s(scc.cause.coprocessorError);
  s(scc.cause.branchTaken);
  s(scc.cause.branchDelay);
  s(scc.epc);

  gte.serialize(s);
}
