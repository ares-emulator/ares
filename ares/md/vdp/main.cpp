auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::tick() -> void {
  if(h32()) {
    auto cycles = &cyclesH32[edclk()][hclock()];
    step(cycles[0] + cycles[1]);
  }
  if(h40()) {
    auto cycles = &cyclesH40[edclk()][hclock()];
    step(cycles[0] + cycles[1]);
  }
  state.hclock += 2;
}

auto VDP::main() -> void {
  scanline();

  if((vcounter() < screenHeight() || vcounter() >= frameHeight() - 2) && io.displayEnable) {
    if(h32()) mainH32Active();
    if(h40()) mainH40Active();
  } else {
    if(h32()) mainH32Blank();
    if(h40()) mainH40Blank();
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

auto VDP::render(bool displayEnable) -> void {
  dac.pixels = pixels();
  if(!dac.pixels) return;
  if(displayEnable) {
    for(auto x : range(screenWidth())) dac.pixel(x);
  } else {
    for(auto x : range(screenWidth())) dac.output(0);
  }
  m32x.vdp.scanline(pixels(), vcounter());
}

auto VDP::hblank(bool line) -> void {
  state.hblank = line;
  if(line == 0) {
    cartridge.hblank(0);
  } else {
    cartridge.hblank(1);
    apu.setINT(0);
    if(vcounter() < screenHeight() - 1
    || vcounter() == frameHeight() - 1
    ) {
      if(irq.hblank.counter-- == 0) {
        irq.hblank.counter = irq.hblank.frequency;
        irq.hblank.pending = 1;
        irq.poll();
      }
    } else {
      irq.hblank.counter = irq.hblank.frequency;
    }
  }
}

auto VDP::vblank(bool line) -> void {
  state.vblank = line;
  if(line == 0) {
    cartridge.vblank(0);
  } else {
    cartridge.vblank(1);
    apu.setINT(1);
    irq.vblank.pending = 1;
    irq.poll();
  }
}

auto VDP::mainH32Active() -> void {
  //0
  hblank(0);

  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch(26);
  tick(); sprite.patternFetch(27);
  tick(); sprite.patternFetch(28);
  tick(); sprite.patternFetch(29);

  //6
  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(0);

  //6-13
  tick(); layerA.mappingFetch(-1);
  tick(); sprite.patternFetch(30);
  tick(); layerA.patternFetch( 0);
  tick(); layerA.patternFetch( 1);
  tick(); layerB.mappingFetch(-1);
  tick(); sprite.patternFetch(31);
  tick(); layerB.patternFetch( 0);
  tick(); layerB.patternFetch( 1);

  //14
  if(vcounter() < frameHeight() - 2) {
    sprite.begin(vcounter(), field());
  } else if(vcounter() == frameHeight() - 2) {
    sprite.begin(-2, field() ^ 1);
  } else {
    sprite.begin(-1, field() ^ 1);
  }

  //14-141
  for(auto block : range(16)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block + 1);
    tick(); layerA.mappingFetch(block);
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch(block * 2 + 2);
    tick(); layerA.patternFetch(block * 2 + 3);
    tick(); layerB.mappingFetch(block);
    tick(); sprite.mappingFetch(block);
    tick(); layerB.patternFetch(block * 2 + 2);
    if(hclock() >> 1 == 132 && vcounter() == screenHeight() - 1) vblank(1);
    if(hclock() >> 1 == 132 && vcounter() == frameHeight()  - 2) vblank(0);
    tick(); layerB.patternFetch(block * 2 + 3);
  }

  //142
  layers.vscrollFetch();
  render(1);
  hblank(1);
  sprite.end();

  //142-171
  tick(); fifo.slot();
  tick(); fifo.slot();
  for(auto cycle : range(13)) {
    tick(); sprite.patternFetch(cycle + 0);
  }
  tick(); fifo.slot();
  for(auto cycle : range(13)) {
    tick(); sprite.patternFetch(cycle + 13);
  }
  tick(); fifo.slot();
}

auto VDP::mainH32Blank() -> void {
  //0
  hblank(0);

  //1-2
  tick();
  tick();

  //3-169
  for(auto cycle : range(167)) {
    tick(); fifo.slot();
    if(hclock() >> 1 == 132 && vcounter() == screenHeight() - 1) vblank(1);
    if(hclock() >> 1 == 132 && vcounter() == frameHeight()  - 2) vblank(0);
    if(hclock() >> 1 == 142) hblank(1);
  }

  //170
  render(0);

  //170-171
  tick();
  tick();
}

auto VDP::mainH40Active() -> void {
  //0
  hblank(0);

  //1-5
  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch(34);
  tick(); sprite.patternFetch(35);
  tick(); sprite.patternFetch(36);
  tick(); sprite.patternFetch(37);

  //6
  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(0);

  //6-13
  tick(); layerA.mappingFetch(-1);
  tick(); sprite.patternFetch(38);
  tick(); layerA.patternFetch( 0);
  tick(); layerA.patternFetch( 1);
  tick(); layerB.mappingFetch(-1);
  tick(); sprite.patternFetch(39);
  tick(); layerB.patternFetch( 0);
  tick(); layerB.patternFetch( 1);

  //14
  if(vcounter() < frameHeight() - 2) {
    sprite.begin(vcounter(), field());
  } else if(vcounter() == frameHeight() - 2) {
    sprite.begin(-2, field() ^ 1);
  } else {
    sprite.begin(-1, field() ^ 1);
  }

  //14-173
  for(auto block : range(20)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block + 1);
    tick(); layerA.mappingFetch(block);
    tick(); if((block & 3) != 3) fifo.slot();
    tick(); layerA.patternFetch(block * 2 + 2);
    tick(); layerA.patternFetch(block * 2 + 3);
    tick(); layerB.mappingFetch(block);
    tick(); sprite.mappingFetch(block);
    tick(); layerB.patternFetch(block * 2 + 2);
    if(hclock() >> 1 == 164 && vcounter() == screenHeight() - 1) vblank(1);
    if(hclock() >> 1 == 164 && vcounter() == frameHeight()  - 2) vblank(0);
    tick(); layerB.patternFetch(block * 2 + 3);
  }

  //174
  layers.vscrollFetch();
  render(1);
  hblank(1);
  sprite.end();

  //174-210
  tick(); fifo.slot();
  tick(); fifo.slot();
  for(auto cycle : range(23)) {
    tick(); sprite.patternFetch(cycle + 0);
  }
  tick(); fifo.slot();
  for(auto cycle : range(11)) {
    tick(); sprite.patternFetch(cycle + 23);
  }
}

auto VDP::mainH40Blank() -> void {
  //0
  hblank(0);

  //1-3
  tick();
  tick();
  tick();

  //4-207
  for(auto cycle : range(204)) {
    tick(); fifo.slot();
    if(hclock() >> 1 == 164 && vcounter() == screenHeight() - 1) vblank(1);
    if(hclock() >> 1 == 164 && vcounter() == frameHeight()  - 2) vblank(0);
    if(hclock() >> 1 == 174) hblank(1);
  }

  //208
  render(0);

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

  //active even half lines
  //======================

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
