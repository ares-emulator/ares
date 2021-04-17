auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto VDP::tick() -> void {
  if(h32()) {
    auto cycles = &cyclesH32[edclk()][state.hclock];
    step(cycles[0] + cycles[1]);
  }
  if(h40()) {
    auto cycles = &cyclesH40[edclk()][state.hclock];
    step(cycles[0] + cycles[1]);
  }
  state.hclock += 2;
}

auto VDP::main() -> void {
  scanline();
  if(vcounter() == frameHeight() - 1) vsync(0);
  if(vcounter() == screenHeight()) vsync(1);

  if(vcounter() < screenHeight() && io.displayEnable) {
    if(h32()) mainH32();
    if(h40()) mainH40();
    for(auto pixel : range(screenWidth())) dac.pixel();
    m32x.vdp.scanline(pixels(), vcounter());
  } else {
    if(h32()) mainBlankH32();
    if(h40()) mainBlankH40();
  }

  state.hclock = 0;
  state.hcounter = 0;
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
  hsync(0);
  layerA.begin();
  layerB.begin();

  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();

  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch();

  //6-13
  tick(); layerA.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerB.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerB.patternFetch();
  tick(); layerB.patternFetch();

  layers.begin();
  window.begin();
  sprite.begin();
  dac.begin();

  //14-141
  for(auto block : range(16)) {
    layers.vscrollFetch();
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch();
    tick(); layerA.mappingFetch();
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch();
    tick(); layerA.patternFetch();
    tick(); layerB.mappingFetch();
    tick(); sprite.mappingFetch();
    tick(); layerB.patternFetch();
    tick(); layerB.patternFetch();
  }

  hsync(1);
  sprite.end();

  //142-171
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
  hsync(0);
  layerA.begin();
  layerB.begin();

  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();
  tick(); sprite.patternFetch();

  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch();

  //6-13
  tick(); layerA.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerA.patternFetch();
  tick(); layerB.mappingFetch();
  tick(); sprite.patternFetch();
  tick(); layerB.patternFetch();
  tick(); layerB.patternFetch();

  layers.begin();
  window.begin();
  sprite.begin();
  dac.begin();

  //14-173
  for(auto block : range(20)) {
    layers.vscrollFetch();
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch();
    tick(); layerA.mappingFetch();
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch();
    tick(); layerA.patternFetch();
    tick(); layerB.mappingFetch();
    tick(); sprite.mappingFetch();
    tick(); layerB.patternFetch();
    tick(); layerB.patternFetch();
  }

  hsync(1);
  sprite.end();

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
}

auto VDP::mainBlankH32() -> void {
  //1-5
  tick();
  tick();
  tick(); fifo.slot();
  tick(); fifo.slot();
  tick(); fifo.slot(); hsync(0);

  //6-148
  for(auto cycle : range(143)) {
    tick(); fifo.slot();
  }

  //149
  tick(); fifo.slot(); hsync(1);

  //150-169
  for(auto cycle : range(20)) {
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
  for(auto cycle : range(192)) {
    tick(); fifo.slot();
  }

  //196
  tick(); hsync(1);

  //197-207
  for(auto cycle : range(11)) {
    tick(); fifo.slot();
  }

  //208-210
  tick();
  tick();
  tick();
}

//timings are approximations; exact positions of slow/normal/fast cycles are not known
auto VDP::generateCycleTimings() -> void {
  //full lines
  //==========

  //H32/DCLK: 342 slow + 0 normal +   0 fast = 3420 cycles
  for(auto cycle : range(342)) cyclesH32[0][cycle *  1] = 10;

  //H32/EDCLK: 21 slow + 3 normal + 318 fast = 2781 cycles
  for(auto cycle : range(342)) cyclesH32[1][cycle *  1] =  8;
  for(auto cycle : range( 24)) cyclesH32[1][cycle * 14] = 10;
  for(auto cycle : range(  3)) cyclesH32[1][cycle * 14] =  9;

  //H40/DCLK:   0 slow + 0 normal + 420 fast = 3360 cycles
  for(auto cycle : range(420)) cyclesH40[0][cycle *  1] =  8;

  //H40/EDCLK: 28 slow + 4 normal + 388 fast = 3420 cycles
  for(auto cycle : range(420)) cyclesH40[1][cycle *  1] =  8;
  for(auto cycle : range( 32)) cyclesH40[1][cycle * 13] = 10;
  for(auto cycle : range(  4)) cyclesH40[1][cycle * 13] =  9;

  //half lines
  //==========

  //H32/DCLK: 171 slow + 0 normal +   0 fast = 1710 cycles
  for(auto cycle : range(171)) halvesH32[0][cycle *  1] = 10;

  //H32/EDCLK: 10 slow + 2 normal + 159 fast = 1390 cycles
  for(auto cycle : range(171)) halvesH32[1][cycle *  1] =  8;
  for(auto cycle : range( 12)) halvesH32[1][cycle * 14] = 10;
  for(auto cycle : range(  2)) halvesH32[1][cycle * 14] =  9;

  //H40/DCLK:   0 slow + 0 normal + 210 fast = 1680 cycles
  for(auto cycle : range(210)) halvesH40[0][cycle *  1] =  8;

  //H40/EDCLK: 14 slow + 2 normal + 194 fast = 1710 cycles
  for(auto cycle : range(210)) halvesH40[1][cycle *  1] =  8;
  for(auto cycle : range( 16)) halvesH40[1][cycle * 13] = 10;
  for(auto cycle : range(  2)) halvesH40[1][cycle * 13] =  9;

  //active even lines
  //=================

  //H32/DCLK: 171 slow + 0 normal +   0 fast = 1710 cycles
  for(auto cycle : range(171)) extrasH32[0][cycle *  1] = 10;

  //H32/EDCLK: 21 slow + 3 normal + 147 fast = 1413 cycles
  for(auto cycle : range(171)) extrasH32[1][cycle *  1] =  8;
  for(auto cycle : range( 24)) extrasH32[1][cycle *  7] = 10;
  for(auto cycle : range(  3)) extrasH32[1][cycle *  7] =  9;

  //H40/DCLK:   0 slow + 0 normal + 210 fast = 1680 cycles
  for(auto cycle : range(171)) extrasH40[0][cycle *  1] =  8;

  //H40/EDCLK: 28 slow + 4 normal + 178 fast = 1740 cycles
  for(auto cycle : range(171)) extrasH40[1][cycle *  1] =  8;
  for(auto cycle : range( 32)) extrasH40[1][cycle *  5] = 10;
  for(auto cycle : range(  4)) extrasH40[1][cycle *  5] =  9;
}
