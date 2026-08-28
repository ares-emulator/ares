auto Harmony::step(u32 clocks) -> void {
  if(running) callCycles += clocks;
  if(timer1Control & 1) timer1Counter += clocks;
  cartridge.stepARM(clocks);
  Thread::step(clocks);
  Thread::synchronize(cpu);
}
