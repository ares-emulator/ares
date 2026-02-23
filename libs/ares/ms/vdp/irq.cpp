auto VDP::IRQ::poll() -> void {
  if(line.pending && line.enable) return cpu.setIRQ(1);
  if(frame.pending && frame.enable) return cpu.setIRQ(1);
  return cpu.setIRQ(0);
}

auto VDP::IRQ::power() -> void {
  line = {};
  frame = {};
}
