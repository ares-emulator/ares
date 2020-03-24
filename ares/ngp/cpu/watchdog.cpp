auto CPU::Watchdog::step(uint clocks) -> void {
  if(!enable) return;
  counter += clocks;
  if(counter < cpu.frequency()) return;
  cpu.intwd.trigger();
}
