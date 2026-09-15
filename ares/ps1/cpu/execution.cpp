auto CPU::retireExecutionUnits() -> void {
  if(execution.multiplyDivide.active && execution.multiplyDivide.completion <= execution.clock) {
    ipu.hi = execution.multiplyDivide.hi;
    ipu.lo = execution.multiplyDivide.lo;
    execution.multiplyDivide = {};
  }

  if(execution.gteCompletion && execution.gteCompletion <= execution.clock) execution.gteCompletion = 0;

  while(true) {
    Execution::Event* next = nullptr;
    for(auto& event : execution.events) {
      if(event.type == Execution::EventNone || event.completion > execution.clock) continue;
      if(!next || event.completion < next->completion
      || (event.completion == next->completion && event.sequence < next->sequence)) next = &event;
    }
    if(!next) break;

    if(next->type == Execution::EventGTEDataWrite) gte.setDataRegister(next->index, next->value);
    if(next->type == Execution::EventGTEControlWrite) gte.setControlRegister(next->index, next->value);
    *next = {};
  }
}

auto CPU::stallMultiplyDivide() -> void {
  if(!execution.multiplyDivide.active) return;
  if(execution.multiplyDivide.completion > execution.clock) {
    step(execution.multiplyDivide.completion - execution.clock);
  }
}

auto CPU::scheduleMultiplyDivide(u32 hi, u32 lo, u32 cycles) -> void {
  execution.multiplyDivide.active = true;
  execution.multiplyDivide.completion = execution.clock + cycles - 1;
  execution.multiplyDivide.hi = hi;
  execution.multiplyDivide.lo = lo;
}

auto CPU::multiplyCycles(s32 rs) const -> u32 {
  if(rs < 0) return rs >= -0x800 ? 6 : rs >= -0x10'0000 ? 9 : 13;
  return rs < 0x800 ? 6 : rs < 0x10'0000 ? 9 : 13;
}

auto CPU::multiplyCycles(u32 rs) const -> u32 {
  return rs < 0x800 ? 6 : rs < 0x10'0000 ? 9 : 13;
}

auto CPU::stallGTE() -> void {
  if(execution.gteCompletion > execution.clock) step(execution.gteCompletion - execution.clock);
}

auto CPU::scheduleGTE(u32 cycles) -> void {
  execution.gteCompletion = execution.clock + cycles - 1;
}

auto CPU::gteCommandCycles(u8 command) const -> u32 {
  return gte.commandCycles(command);
}

auto CPU::scheduleGTEDataWrite(u8 index, u32 value) -> void {
  scheduleExecutionEvent(Execution::EventGTEDataWrite, index, value, index == 28 ? 2 : 1);
}

auto CPU::scheduleGTEControlWrite(u8 index, u32 value) -> void {
  scheduleExecutionEvent(Execution::EventGTEControlWrite, index, value, 2);
}

auto CPU::scheduleExecutionEvent(u8 type, u8 index, u32 value, u32 delaySlots) -> void {
  Execution::Event* target = nullptr;
  while(!target) {
    for(auto& event : execution.events) {
      if(event.type == Execution::EventNone) {
        target = &event;
        break;
      }
    }
    if(target) break;

    u64 completion = ~u64(0);
    for(auto& event : execution.events) completion = min(completion, event.completion);
    step(completion - execution.clock);
  }
  //The caller supplies complete following boundaries that must still observe the old value.
  //Retire at the next boundary, before its dependent decoder can observe the event.
  *target = {type, index, value, execution.clock + delaySlots + 1, execution.nextEventSequence++};
}

auto CPU::scheduleStatusVisibility(u32 previous, u32 value, u64 retirement) -> void {
  if(!execution.status.managed) {
    execution.status.managed = true;
    execution.status.visible = previous;
  }

  for(auto& event : execution.status.events) {
    if(event.active) continue;
    event = {true, value, retirement, execution.status.nextSequence++};
    return;
  }
  unreachable;
}

auto CPU::retireStatusVisibility() -> void {
  while(true) {
    Execution::StatusVisibility::Event* next = nullptr;
    for(auto& event : execution.status.events) {
      if(!event.active || event.retirement > execution.retiredInstructions) continue;
      if(!next || event.retirement < next->retirement
      || event.retirement == next->retirement && event.sequence < next->sequence) next = &event;
    }
    if(!next) return;
    execution.status.visible = next->value;
    *next = {};
  }
}

auto CPU::latestStatusVisibilityRetirement() const -> u64 {
  u64 retirement = 0;
  for(auto& event : execution.status.events) {
    if(event.active) retirement = max(retirement, event.retirement);
  }
  return retirement;
}

auto CPU::synchronizeStatusVisibility() -> void {
  execution.status.managed = true;
  execution.status.visible = statusRegisterSCC();
  for(auto& event : execution.status.events) event = {};
}

auto CPU::coprocessor2Enabled() const -> bool {
  return statusCoprocessorEnabled(2);
}
