auto CPU::Watchdog::step(u32 clocks) -> void {
  if(!enable) return;
  counter += clocks;
  if(counter < cpu.frequency()) return;
  counter -= cpu.frequency();
  cpu.intwd.trigger();
}
