auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::tick() -> void {
  step(cycles[0] + cycles[1]);
  cycles += 2;
  state.hcounter++;
  if(h32()) {
    if(hcounter() == 0x00) hblank(0), vedge();
    if(hcounter() == 0x81) vtick();
    if(hcounter() == 0x93) hblank(1);
    if(hcounter() == 0x94) state.hcounter = 0xe9;
  }
  if(h40()) {
    if(hcounter() == 0x00) hblank(0), vedge();
    if(hcounter() == 0xa1) vtick();
    if(hcounter() == 0xb3) hblank(1);
    if(hcounter() == 0xb6) state.hcounter = 0xe4;
  }
  vram.refreshing = 0;
}

auto VDP::vtick() -> void {
  if(vblank()) {
    irq.hblank.counter = irq.hblank.frequency;
  } else if(irq.hblank.counter-- == 0) {
    irq.hblank.counter = irq.hblank.frequency;
    irq.hblank.pending = 1;
    irq.poll();
  }

  state.vcounter++;
  if(v28()) {
    if(vcounter() == 0x0e0) vblank(1);
    if(vcounter() == 0x0eb && Region::NTSC()) state.vcounter = 0x1e5;
    if(vcounter() == 0x103 && Region::PAL ()) state.vcounter = 0x1ca;
    if(vcounter() == 0x1ff) vblank(0);
  }
  if(v30()) {
    if(vcounter() == 0x0f0) vblank(1);
    if(vcounter() == 0x200 && Region::NTSC()) state.vcounter = 0x000;
    if(vcounter() == 0x10b && Region::PAL ()) state.vcounter = 0x1d2;
    if(vcounter() == 0x1ff) vblank(0);
  }
}

auto VDP::hblank(bool line) -> void {
  state.hblank = line;
  if(hblank() == 0) {
    cartridge.hblank(0);
  } else {
    cartridge.hblank(1);
    apu.setINT(0);  //timing hack
  }
}

auto VDP::vblank(bool line) -> void {
  state.vblank = line;
  irq.vblank.transitioned = 1;
}

auto VDP::vedge() -> void {
  if(!irq.vblank.transitioned) return;
  irq.vblank.transitioned = 0;

  if(vblank() == 0) {
    cartridge.vblank(0);
  //apu.setINT(0);
  } else {
    cartridge.vblank(1);
    apu.setINT(1);
    irq.vblank.pending = 1;
    irq.poll();
  }
}

auto VDP::slot() -> void {
  if(state.refreshing) {
    state.refreshing = 0;
    return;
  }
  if(!fifo.run()) prefetch.run();
  dma.run();
}

auto VDP::refresh() -> void {
  vram.refreshing  = 1;
  state.refreshing = !displayEnable();
}

auto VDP::main() -> void {
  latch.displayWidth = io.displayWidth;
  latch.clockSelect  = io.clockSelect;
  if(h32()) mainH32();
  if(h40()) mainH40();
  if(vcounter() == 0) {
    latch.interlace = io.interlaceMode == 3;
    latch.overscan  = io.overscan;
    frame();
    state.field ^= 1;
  }
}

auto VDP::mainH32() -> void {
  auto pixels = dac.pixels = vdp.pixels();
  auto scanline = vcounter();
  cycles = &cyclesH32[edclk()][0];

  sprite.begin();
  for(auto block : range(16)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block);
    tick(); layerA.mappingFetch(block);
    tick(); (block & 3) != 3 ? slot() : refresh();
    tick(); layerA.patternFetch(block * 2 + 2);
    tick(); layerA.patternFetch(block * 2 + 3);
    tick(); layerB.mappingFetch(block);
    tick(); sprite.mappingFetch(block);
    tick(); layerB.patternFetch(block * 2 + 2);
    tick(); layerB.patternFetch(block * 2 + 3);
    for(auto pixel : range(16)) dac.pixel(block * 16 + pixel);
  }
  m32x.vdp.scanline(pixels, scanline);

  tick(); slot();
  tick(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(13)) {
    tick(); sprite.patternFetch(cycle + 0);
  }
  tick(); displayEnable() ? slot() : refresh();
  for(auto cycle : range(13)) {
    tick(); sprite.patternFetch(cycle + 13);
  }
  tick(); slot();

  layerA.begin();
  layerB.begin();

  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch(26);
  tick(); sprite.patternFetch(27);
  tick(); sprite.patternFetch(28);
  tick(); sprite.patternFetch(29);

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick(); layerA.mappingFetch(-1);
  tick(); sprite.patternFetch(30);
  tick(); layerA.patternFetch( 0);
  tick(); layerA.patternFetch( 1);
  tick(); layerB.mappingFetch(-1);
  tick(); sprite.patternFetch(31);
  tick(); layerB.patternFetch( 0);
  tick(); layerB.patternFetch( 1);
}

auto VDP::mainH40() -> void {
  auto pixels = dac.pixels = vdp.pixels();
  cycles = &cyclesH40[edclk()][0];

  sprite.begin();
  for(auto block : range(20)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block);
    tick(); layerA.mappingFetch(block);
    tick(); (block & 3) != 3 ? slot() : refresh();
    tick(); layerA.patternFetch(block * 2 + 2);
    tick(); layerA.patternFetch(block * 2 + 3);
    tick(); layerB.mappingFetch(block);
    tick(); sprite.mappingFetch(block);
    tick(); layerB.patternFetch(block * 2 + 2);
    tick(); layerB.patternFetch(block * 2 + 3);
    for(auto pixel : range(16)) dac.pixel(block * 16 + pixel);
  }
  m32x.vdp.scanline(pixels, vcounter());

  tick(); slot();
  tick(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(23)) {
    tick(); sprite.patternFetch(cycle + 0);
  }
  tick(); displayEnable() ? slot() : refresh();
  for(auto cycle : range(11)) {
    tick(); sprite.patternFetch(cycle + 23);
  }

  layerA.begin();
  layerB.begin();

  tick(); layers.hscrollFetch();
  tick(); sprite.patternFetch(34);
  tick(); sprite.patternFetch(35);
  tick(); sprite.patternFetch(36);
  tick(); sprite.patternFetch(37);

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick(); layerA.mappingFetch(-1);
  tick(); sprite.patternFetch(38);
  tick(); layerA.patternFetch( 0);
  tick(); layerA.patternFetch( 1);
  tick(); layerB.mappingFetch(-1);
  tick(); sprite.patternFetch(39);
  tick(); layerB.patternFetch( 0);
  tick(); layerB.patternFetch( 1);
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

  cycles = nullptr;
}
