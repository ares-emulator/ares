auto VDP::IRQ::poll() -> void {
  if(hblank.enable && hblank.pending) {
    hblank.pending = 0;
    cpu.raise(CPU::Interrupt::HorizontalBlank);
  }

  if(vblank.enable && vblank.pending) {
    vblank.pending = 0;
    cpu.raise(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::IRQ::power(bool reset) -> void {
  external = {};
  hblank = {};
  vblank = {};
}
