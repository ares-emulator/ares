auto VDP::IRQ::poll() -> void {
  if(external.enable && external.pending) {
    vdp.debugger.interrupt("External");
    external.pending = 0;
    cpu.raise(CPU::Interrupt::External);
  }

  if(hblank.enable && hblank.pending) {
    vdp.debugger.interrupt("Hblank");
    hblank.pending = 0;
    cpu.raise(CPU::Interrupt::HorizontalBlank);
  }

  if(vblank.enable && vblank.pending) {
    vdp.debugger.interrupt("Vblank");
    vblank.pending = 0;
    cpu.raise(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::IRQ::power(bool reset) -> void {
  external = {};
  hblank = {};
  vblank = {};
}
