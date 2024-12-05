auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

template<bool _h40> auto VDP::tick() -> void {
  step(cycles[0] + cycles[1]);
  cycles += 2;
  state.hcounter++;

  if(_h40) {
    if(hcounter() == 0x00) hblank(0), vedge();
    else if(hcounter() == 0xa3) vtick();
    else if(hcounter() == 0xb3) hblank(1);
    else if(hcounter() == 0xb6) state.hcounter = 0xe4;
  } else {
    if(hcounter() == 0x00) hblank(0), vedge();
    else if(hcounter() == 0x83) vtick();
    else if(hcounter() == 0x93) hblank(1);
    else if(hcounter() == 0x94) state.hcounter = 0xe9;
  }

  irq.poll();
}

auto VDP::vblankcheck() -> void {
  if(v28()) {
    if(vcounter() == 0x0e0) vblank(1);
    if(vcounter() == 0x1ff) vblank(0);
  }
  if(v30()) {
    if(vcounter() == 0x0f0) vblank(1);
    if(vcounter() == 0x1ff) vblank(0);
  }
}

auto VDP::vtick() -> void {
  if(vblank()) {
    irq.hblank.counter = irq.hblank.frequency;
  } else if(irq.hblank.counter-- == 0) {
    irq.hblank.counter = irq.hblank.frequency;
    irq.hblank.pending = 1;
    debugger.interrupt(CPU::Interrupt::HorizontalBlank);
  }

  state.vcounter++;
  if(v28()) {
    if(vcounter() == 0x0eb && Region::NTSC()) state.vcounter = 0x1e5;
    if(vcounter() == 0x103 && Region::PAL ()) state.vcounter = 0x1ca;
  }
  if(v30()) {
    if(vcounter() == 0x200 && Region::NTSC()) state.vcounter = 0x000;
    if(vcounter() == 0x10b && Region::PAL ()) state.vcounter = 0x1d2;
  }

  vblankcheck();
}

auto VDP::hblank(bool line) -> void {
  state.hblank = line;
  if(hblank() == 0) {
    cartridge.hblank(0);
  } else {
    cartridge.hblank(1);
  }
}

auto VDP::vblank(bool line) -> void {
  irq.vblank.transitioned |= state.vblank ^ line;
  state.vblank = line;
}

auto VDP::vedge() -> void {
  apu.setINT(0);
  if(!irq.vblank.transitioned) return;
  irq.vblank.transitioned = 0;

  if(vblank() == 0) {
    cartridge.vblank(0);
  } else {
    cartridge.vblank(1);
    apu.setINT(1);
    irq.vblank.pending = 1;
    debugger.interrupt(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::slot() -> void {
  if(!fifo.run()) prefetch.run();
  dma.run();
}

auto VDP::refresh(bool active) -> void {
  vram.refreshing = active;
}

auto VDP::main() -> void {
  latch.displayWidth = io.displayWidth;
  latch.clockSelect  = io.clockSelect;
  if(h32()) mainH32();
  if(h40()) mainH40();
  if(vcounter() == 0) {
    screen->setColorBleedWidth(latch.displayWidth ? 4 : 5);
    latch.interlace = io.interlaceMode == 3;
    latch.overscan  = io.overscan;
    frame();
    state.field ^= 1;
  }
}

auto VDP::mainH32() -> void {
  auto pixels = dac.pixels = vdp.pixels();
  cycles = &cyclesH32[edclk()][0];

  sprite.begin();
  if(dac.pixels) blocks<false, true>();
  else blocks<false, false>();

  if(Mega32X()) m32x.vdp.scanline(pixels, vcounter());

  tick<false>(); slot();
  tick<false>(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(13)) {
    tick<false>(); sprite.patternFetch(cycle + 0);
  }
  tick<false>(); slot();
  for(auto cycle : range(13)) {
    tick<false>(); sprite.patternFetch(cycle + 13);
  }
  tick<false>(); slot();

  layerA.begin();
  layerB.begin();
  window.begin();

  tick<false>(); layers.hscrollFetch();
  tick<false>(); sprite.patternFetch(26);
  tick<false>(); sprite.patternFetch(27);
  tick<false>(); sprite.patternFetch(28);
  tick<false>(); sprite.patternFetch(29);

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick<false>(); layerA.mappingFetch(-1);
  tick<false>(); !displayEnable() ? refresh(true) : sprite.patternFetch(30);
  tick<false>(); layerA.patternFetch( 0); refresh(false);
  tick<false>(); layerA.patternFetch( 1);
  tick<false>(); layerB.mappingFetch(-1);
  tick<false>(); sprite.patternFetch(31);
  tick<false>(); layerB.patternFetch( 0);
  tick<false>(); layerB.patternFetch( 1);
}

auto VDP::mainH40() -> void {
  auto pixels = dac.pixels = vdp.pixels();
  cycles = &cyclesH40[edclk()][0];

  sprite.begin();
  if(dac.pixels) blocks<true, true>();
  else blocks<true, false>();

  if(Mega32X()) m32x.vdp.scanline(pixels, vcounter());

  tick<true>(); slot();
  tick<true>(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(23)) {
    tick<true>(); sprite.patternFetch(cycle + 0);
  }
  tick<true>(); slot();
  for(auto cycle : range(11)) {
    tick<true>(); sprite.patternFetch(cycle + 23);
  }

  layerA.begin();
  layerB.begin();
  window.begin();

  tick<true>(); layers.hscrollFetch();
  tick<true>(); sprite.patternFetch(34);
  tick<true>(); sprite.patternFetch(35);
  tick<true>(); sprite.patternFetch(36);
  tick<true>(); sprite.patternFetch(37);

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick<true>(); layerA.mappingFetch(-1);
  tick<true>(); !displayEnable() ? refresh(true) : sprite.patternFetch(38);
  tick<true>(); layerA.patternFetch( 0); refresh(false);
  tick<true>(); layerA.patternFetch( 1);
  tick<true>(); layerB.mappingFetch(-1);
  tick<true>(); sprite.patternFetch(39);
  tick<true>(); layerB.patternFetch( 0);
  tick<true>(); layerB.patternFetch( 1);
}

template<bool _h40, bool _pixels> auto VDP::blocks() -> void {
  bool den = displayEnable();
  bool vc = vcounter() == 0x1ff;
  for(auto block : range(_h40 ? 20 : 16)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block);
    tick<_h40>(); layerA.mappingFetch(block);
    tick<_h40>(); (block & 3) != 3 ? slot() : refresh(true);
    tick<_h40>(); layerA.patternFetch(block * 2 + 2); refresh(false);
    tick<_h40>(); layerA.patternFetch(block * 2 + 3);
    tick<_h40>(); layerB.mappingFetch(block);
    tick<_h40>(); sprite.mappingFetch(block);
    tick<_h40>(); layerB.patternFetch(block * 2 + 2);
    tick<_h40>(); layerB.patternFetch(block * 2 + 3);

    if(_pixels) {
      if(!den || vc) {
        for(auto pixel: range(16)) dac.pixel<_h40, false>(block * 16 + pixel);
      } else {
        for(auto pixel: range(16)) dac.pixel<_h40, true>(block * 16 + pixel);
      }
    }
  }
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
