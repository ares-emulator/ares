auto VDP::IRQ::poll() -> void {
  if(external.enable && external.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::External);
    external.pending = 0;
    cpu.raise(CPU::Interrupt::External);
  } else {
    external.pending = 0;
    cpu.lower(CPU::Interrupt::External);
  }

  if(hblank.enable && hblank.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::HorizontalBlank);
    hblank.pending = 0;
    cpu.raise(CPU::Interrupt::HorizontalBlank);
  } else {
    hblank.pending = 0;
    cpu.lower(CPU::Interrupt::HorizontalBlank);
  }

  if(vblank.enable && vblank.pending) {
    vdp.debugger.interrupt(CPU::Interrupt::VerticalBlank);
    vblank.pending = 0;
    cpu.raise(CPU::Interrupt::VerticalBlank);
  } else {
    vblank.pending = 0;
    cpu.lower(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::IRQ::power(bool reset) -> void {
  external = {};
  hblank = {};
  vblank = {};
}
