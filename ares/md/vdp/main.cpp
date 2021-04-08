auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::tick() -> void {
  state.hdot++;
  if(h32()) step(20);
  if(h40()) step(16);
  if(edclk() && ++state.ecounter == 7) {
    state.ecounter = 0;
    step(2);
  }
}

auto VDP::main() -> void {
  scanline();
  if(vcounter() == frameHeight() - 1) vsync(0);
  if(vcounter() == screenHeight()) vsync(1);

  if(vcounter() < screenHeight() && io.displayEnable) {
    if(h32()) mainH32();
    if(h40()) mainH40();
    u32 hdot = state.hdot;
    for(state.hdot = 0; state.hdot < screenWidth(); state.hdot++) run();
    state.hdot = hdot;
    m32x.vdp.scanline(pixels(), vcounter());
  } else {
    if(h32()) mainBlankH32();
    if(h40()) mainBlankH40();
  }

  state.hdot = 0;
  state.hcounter = 0;
  state.ecounter = 0;
  latch.displayWidth = io.displayWidth;
  latch.clockSelect = io.clockSelect;

  if(++state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.interlace = io.interlaceMode == 3;
    latch.overscan = io.overscan;
  }
}

auto VDP::hsync(bool line) -> void {
  state.hsync = line;
  if(line == 0) {
    cartridge.hblank(0);
    cpu.lower(CPU::Interrupt::HorizontalBlank);
  } else {
    cartridge.hblank(1);
    apu.setINT(false);
    if(vcounter() < screenHeight()) {
      if(latch.hblankInterruptCounter-- == 0) {
        latch.hblankInterruptCounter = io.hblankInterruptCounter;
        if(io.hblankInterruptEnable) {
          cpu.raise(CPU::Interrupt::HorizontalBlank);
        }
      }
    }
  }
}

auto VDP::vsync(bool line) -> void {
  state.vsync = line;
  if(line == 0) {
    io.vblankInterruptTriggered = false;
    latch.hblankInterruptCounter = io.hblankInterruptCounter;
    cartridge.vblank(0);
    cpu.lower(CPU::Interrupt::VerticalBlank);
  } else {
    cartridge.vblank(1);
    apu.setINT(true);
    if(io.vblankInterruptEnable) {
      io.vblankInterruptTriggered = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }
  }
}

auto VDP::mainH32() -> void {
  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch(); hsync(0);

  //6-13
  tick(); layerA.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerB.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerB.patternFetch();
  tick(); layerB.patternFetch();

  //14-141
  for(u32 block : range(16)) {
    tick(); layerA.mappingFetch();
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch();
    tick(); layerA.patternFetch();
    tick(); layerB.mappingFetch();
    tick(); sprite.mappingFetch();
    tick(); layerB.patternFetch();
    tick(); layerB.patternFetch();
  }

  //142-171
  tick(); fifo.slot();
  tick(); fifo.slot();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch(); hsync(1);
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); fifo.slot();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); fifo.slot();
}

auto VDP::mainH40() -> void {
  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch(); hsync(0);
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();

  //6-13
  tick(); layerA.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerB.mappingFetch();
  tick(); sprite.mappingFetch();
  tick(); layerB.patternFetch();
  tick(); layerB.patternFetch();

  //14-173
  for(u32 block : range(20)) {
    tick(); layerA.mappingFetch();
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch();
    tick(); layerA.patternFetch();
    tick(); layerB.mappingFetch();
    tick(); sprite.mappingFetch();
    tick(); layerB.patternFetch();
    tick(); layerB.patternFetch();
  }

  //174-210
  tick(); fifo.slot();
  tick(); fifo.slot();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch(); hsync(1);
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); fifo.slot();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
}

auto VDP::mainBlankH32() -> void {
  //1-5
  tick();
  tick();
  tick(); fifo.slot();
  tick(); fifo.slot();
  tick(); fifo.slot(); hsync(0);

  //6-148
  for(u32 cycle : range(143)) {
    tick(); fifo.slot();
  }

  //149
  tick(); fifo.slot(); hsync(1);

  //150-169
  for(u32 cycle : range(20)) {
    tick(); fifo.slot();
  }

  //170-171
  tick();
  tick();
}

auto VDP::mainBlankH40() -> void {
  //1-3
  tick();
  tick(); hsync(0);
  tick();

  //4-195
  for(u32 cycle : range(192)) {
    tick(); fifo.slot();
  }

  //196
  tick(); hsync(1);

  //197-207
  for(u32 cycle : range(11)) {
    tick(); fifo.slot();
  }

  //208-210
  tick();
  tick();
  tick();
}
