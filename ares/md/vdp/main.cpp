auto VDP::main() -> void {
  scanline();

  if(state.vcounter == screenHeight()) {
    if(io.verticalBlankInterruptEnable) {
      io.vblankIRQ = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }
    cartridge.vblank(1);
    apu.setINT(true);
  }

  if(state.vcounter < screenHeight() && io.displayEnable) {
    while(state.hcounter < 1280) {
      run();
      state.hdot++;
      if((state.hdot & 15) == 0) fifo.slot();
      step(pixelWidth());
    }
    m32x.vdp.scanline(pixels(), state.vcounter);

    if(latch.horizontalInterruptCounter-- == 0) {
      latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
      if(io.horizontalBlankInterruptEnable) {
        cpu.raise(CPU::Interrupt::HorizontalBlank);
      }
    }
  } else {
    for(u32 n : range(256)) {
      fifo.slot();
      step(5);
    }
  }

  cartridge.hblank(1);
  apu.setINT(false);
  for(u32 n : range(43)) {
    fifo.slot();
    step(10);
  }

  cpu.lower(CPU::Interrupt::HorizontalBlank);
  cartridge.hblank(0);

  state.hdot = 0;
  state.hcounter = 0;
  latch.displayWidth = io.displayWidth;

  if(++state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.field = state.field;
    latch.interlace = io.interlaceMode == 3;
    latch.overscan = io.overscan;
    latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
    io.vblankIRQ = false;
    cpu.lower(CPU::Interrupt::VerticalBlank);
    cartridge.vblank(0);
  }
}

auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}
