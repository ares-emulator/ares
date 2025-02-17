auto VDP::IRQ::poll() -> void {
  if(delay) { delay--; return; }

  if(external.enable && external.pending) {
    cpu.raise(CPU::Interrupt::External);
  } else {
    cpu.lower(CPU::Interrupt::External);
  }

  if(hblank.enable && hblank.pending) {
    cpu.raise(CPU::Interrupt::HorizontalBlank);
  } else {
    cpu.lower(CPU::Interrupt::HorizontalBlank);
  }

  if(vblank.enable && vblank.pending) {
    cpu.raise(CPU::Interrupt::VerticalBlank);
  } else {
    cpu.lower(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::IRQ::acknowledge(u8 level) -> void {
  if(level == 2) external.pending = 0;
  if(level == 4) {
    // H-INT ack can errantly cancel V-INT if they coincide.
    // Required for Fatal Rewind et al.
    if(vblank.pending && vblank.enable)
      vblank.pending = 0;
    else
      hblank.pending = 0;
  }
  if(level == 6 && vblank.enable) vblank.pending = 0;
}

auto VDP::IRQ::power(bool reset) -> void {
  external = {};
  hblank = {};
  vblank = {};
  delay = 0;
}
