auto CPU::poll() -> void {
  if(!state.poll || !state.interrupt) return;

  io.interruptStatus |= (io.interruptLevel & io.interruptEnable);

  for(auto id : reverse(range(8))) {
    if(!io.interruptStatus.bit(id)) continue;

    if(interrupt((io.interruptBase & ~7) | id)) {
      debugger.interrupt(id);
    }
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

auto CPU::irqLevel(n3 irq, bool value) -> void {
  io.interruptLevel.bit(irq) = value;
}
