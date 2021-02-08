auto CPU::poll() -> void {
  if(!state.poll) return;

  for(s32 n = 7; n >= 0; n--) {
    if(!r.interruptEnable.bit(n)) continue;
    if(!r.interruptStatus.bit(n)) continue;
    state.halt = false;
    if(!V30MZ::r.f.i) continue;
    static const string type[8] = {
      "SerialSend", "Input", "Cartridge", "SerialReceive",
      "LineCompare", "VblankTimer", "Vblank", "HblankTimer"
    };
    debugger.interrupt(type[n]);
    interrupt(r.interruptBase + n);
    return;
  }
}

auto CPU::raise(Interrupt irq) -> void {
  if(!r.interruptEnable.bit((u32)irq)) return;
  r.interruptStatus.bit((u32)irq) = 1;
}

auto CPU::lower(Interrupt irq) -> void {
  r.interruptStatus.bit((u32)irq) = 0;
}
