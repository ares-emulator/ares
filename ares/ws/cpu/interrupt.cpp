auto CPU::poll() -> void {
  if(!state.poll) return;

  for(auto id : reverse(range(8))) {
    if(!io.interruptEnable.bit(id)) continue;
    if(!io.interruptStatus.bit(id)) continue;
    state.halt = false;
    if(!PSW.IE) continue;

    debugger.interrupt(id);
    interrupt(io.interruptBase + id);
    return;
  }
}

auto CPU::raise(n3 irq) -> void {
  if(!io.interruptEnable.bit(irq)) return;
  io.interruptStatus.bit(irq) = 1;
}

auto CPU::lower(n3 irq) -> void {
  io.interruptStatus.bit(irq) = 0;
}
