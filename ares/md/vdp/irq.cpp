auto VDP::IRQ::poll() -> void {
  if(external.enable && external.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::External);
    cpu.raise(CPU::Interrupt::External);
  } else {
    external.pending = 0;
    cpu.lower(CPU::Interrupt::External);
  }

  if(hblank.enable && hblank.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::HorizontalBlank);
    cpu.raise(CPU::Interrupt::HorizontalBlank);
  } else {
    // note: pending H-INT is not cleared when disabled
    cpu.lower(CPU::Interrupt::HorizontalBlank);
  }

  if(vblank.enable && vblank.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::VerticalBlank);
    cpu.raise(CPU::Interrupt::VerticalBlank);
  } else {
    vblank.pending = 0;
    cpu.lower(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::IRQ::acknowledge(u8 level) -> void {
  if(level == 2) external.pending = 0;
  if(level == 4) hblank.pending = 0;
  if(level == 6) vblank.pending = 0;
}

auto VDP::IRQ::power(bool reset) -> void {
  external = {};
  hblank = {};
  vblank = {};
}
