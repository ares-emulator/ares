auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

template<bool _h40> auto VDP::fullslotStep() -> void {
  // EDCLK hybrid rate : 15cyc @ MClk/5 + 2cyc @ MClk/4
  static u8 EDCLK[17] = {5,5,5,5,5, 5,5,5,5,5, 5,5,5,5,5, 4,4};
  if(( _h40 && latch.clockSelect && hcounter() >= 0xe6 && hcounter() <= 0xf6) ||
     (!_h40 && latch.clockSelect && hcounter() >= 0xec && hcounter() <= 0xfc)) {
    u32 q1 = EDCLK[state.edclkPos];
    state.edclkPos = (state.edclkPos+1) % 17;
    u32 q2 = EDCLK[state.edclkPos];
    state.edclkPos = (state.edclkPos+1) % 17;
    u32 q3 = EDCLK[state.edclkPos];
    state.edclkPos = (state.edclkPos+1) % 17;
    u32 q4 = EDCLK[state.edclkPos];
    state.edclkPos = (state.edclkPos+1) % 17;
    step(q1+q2+q3+q4);
    return;
  }
  if(_h40)
    step(4+4+4+4); // MClk/4
  else
    step(5+5+5+5); // MClk/5
}

template<bool _h40, bool _refresh> auto VDP::tick() -> void {
  // Run DMA here -- fifo & prefetch have ram priority, so somes ops may be blocked
  dma.run();

  fullslotStep<_h40>();
  htick<_h40>(); // +2 pixels

  if(cram.bus.active) {
    vdp.dac.dot(hcounter()*2+1, cram.bus.data);

    // DAC dot artifacts may be drawn continuously in the case of consectutive writes.
    // We can detect for this by checking for an impending CRAM write thru the fifo.
    // If refresh or fifo delay occurs, the data will not be updated, resulting in an extended dot.
    // Note: we're not currently checking for back-to-back slots when display is enabled.
    if(displayEnable() || fifo.slots[0].empty() || fifo.slots[0].target != 3)
      cram.bus.active = 0;
    else {
      if(fifo.slots[0].latency > 1 || vram.refreshing) cram.bus.data = vdp.cram.color(vdp.io.backgroundColor);
      vdp.dac.dot(hcounter()*2+2, cram.bus.data);
    }
  }

  // There is reportedly a latch effect when enabling the display, but it might be a fixed delay
  // rather than a wait until the fifo clears. So, this is precautionary and not necessily correct.
  if(latch.displayEnable > io.displayEnable || fifo.empty())
    latch.displayEnable = io.displayEnable;

  if(_refresh) {
    vram.refreshing = 1;

    // The start of a DMA load will be aligned if it coincides with a refresh slot.
    // The duration may differ between H32 & H40 due to pixel clock, or this may just
    // be the result of some other emulation inaccuracy. Either way, this works for now.
    if(dma.active && dma.preload > 0) dma.preload = h40()?6:4;
  } else {
    fifo.tick();

    // When display is blanked, DMA load fetch may be performed in every slot
    // except for refresh slots and any slot immediately following refresh.
    if(dma.active && !vram.refreshing) dma.fetch();
    vram.refreshing = 0;

    if(dma.active && dma.preload > 0) dma.preload--;
  }

  state.rambusy = 1;
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

template<bool _h40> auto VDP::htick() -> void {
  state.hcounter++;

  if(_h40) {
    if(hcounter() == 0x00) vedge();
    else if(hcounter() == 0x05) hblank(0);
    else if(hcounter() == 0xa5) vtick();
    else if(hcounter() == 0xb3) hblank(1);
    else if(hcounter() == 0xb6) state.hcounter = 0xe4;
  } else {
    if(hcounter() == 0x00) vedge();
    else if(hcounter() == 0x05) hblank(0);
    else if(hcounter() == 0x85) vtick();
    else if(hcounter() == 0x93) hblank(1);
    else if(hcounter() == 0x94) state.hcounter = 0xe9;
  }

  irq.poll();
}

auto VDP::vtick() -> void {
  if(vblank()) {
    irq.hblank.counter = irq.hblank.frequency;
  } else if(irq.hblank.counter-- == 0) {
    irq.hblank.counter = irq.hblank.frequency;
    irq.hblank.pending = 1;
    irq.delay = h40() ? 3 : 2; // 4-6 pixel delay (~6 M68k cycles)
    debugger.interrupt(CPU::Interrupt::HorizontalBlank);
  }

  if(vcounter() == state.bottomline)
    state.vcounter = state.topline;
  else
    state.vcounter++;

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
  if(state.vblank > line) state.topline = vcounter();
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
    irq.delay = h40() ? 3 : 2; // 4-6 pixel delay (~6 M68k cycles)
    debugger.interrupt(CPU::Interrupt::VerticalBlank);
  }
}

auto VDP::slot() -> void {
  state.rambusy = 0;
  if(!(state.rambusy = fifo.run()))
    state.rambusy = prefetch.run();
}

auto VDP::main() -> void {
  latch.displayWidth = io.displayWidth;
  latch.clockSelect  = io.clockSelect;
  state.edclkPos = 0;
  if(h32()) mainH32();
  else
  if(h40()) mainH40();
  if(vcounter() == state.bottomline) {
    screen->setColorBleedWidth(latch.displayWidth ? 4 : 5);
    latch.interlace = io.interlaceMode.bit(0);
    latch.overscan  = io.overscan;
    frame();
    state.field ^= 1;
    updateScreenParams();
  }
}

auto VDP::mainH32() -> void {
  dac.pixels = vdp.pixels();
  auto pixels = dac.active = dac.pixels+13*5;
  state.hcounter = 0;

  sprite.begin();
  if(dac.pixels) {
    blocks<false, true>();
    if(Mega32X()) m32x.vdp.scanline(pixels + 13, vcounter()); //approx 3 and 1/4 pixel offset in H40 pixels
    if(MegaLD()) mcd.ld.scanline(dac.pixels, vcounter());
  } else {
    blocks<false, false>();
    if(MegaLD()) mcd.ld.scanline(dac.pixels, vcounter());
  }

  tick<false>(); slot();
  tick<false>(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(4)) {
    tick<false>(); sprite.patternFetch(cycle + 0);
  }
  for(auto cycle : range(13)) {
    tick<false>(); sprite.patternFetch(cycle + 4); sprite.scan();
  }
  // Placement of this free slot conflicts with documentation by Nemesis which has it 4 slots earlier,
  // but this works more reliably with the Direct Color DMA demos.
  tick<false>(); slot();
  // window begin call (reg latch) is placed here due to garbage line edge case in International Superstar Soccer Deluxe (E)
  window.begin();
  for(auto cycle : range(9)) {
    tick<false>(); sprite.patternFetch(cycle + 17); sprite.scan();
  }
  tick<false>(); slot();

  layerA.begin();
  layerB.begin();

  tick<false>(); layers.hscrollFetch();
  tick<false>(); sprite.patternFetch(26); sprite.scan();
  tick<false>(); sprite.patternFetch(27); sprite.scan();
  tick<false>(); sprite.patternFetch(28); sprite.scan();
  tick<false>(); sprite.patternFetch(29); sprite.scan();

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick<false>(); layerA.mappingFetch(-1);
  if(!displayEnable()) {
    tick<false,true>(); //refresh
  } else {
    tick<false>(); sprite.patternFetch(30); sprite.scan();
  }
  tick<false>(); layerA.patternFetch( 0); sprite.scan();
  tick<false>(); layerA.patternFetch( 1); sprite.scan();
  tick<false>(); layerB.mappingFetch(-1);
  tick<false>(); sprite.patternFetch(31); sprite.scan();
  tick<false>(); layerB.patternFetch( 0); sprite.scan();
  tick<false>(); layerB.patternFetch( 1); sprite.scan();
}

auto VDP::mainH40() -> void {
  dac.pixels = vdp.pixels();
  auto pixels = dac.active = dac.pixels+13*4;
  state.hcounter = 0;

  sprite.begin();
  if(dac.pixels) {
    blocks<true, true>();
    if(Mega32X()) m32x.vdp.scanline(pixels, vcounter());
    if(MegaLD()) mcd.ld.scanline(dac.pixels, vcounter());
  } else {
    blocks<true, false>();
    if(MegaLD()) mcd.ld.scanline(dac.pixels, vcounter());
  }

  tick<true>(); slot();
  tick<true>(); slot();

  layers.vscrollFetch();
  sprite.end();

  for(auto cycle : range(4)) {
    tick<true>(); sprite.patternFetch(cycle + 0);
  }
  for(auto cycle : range(19)) {
    tick<true>(); sprite.patternFetch(cycle + 4); sprite.scan();
  }
  tick<true>(); slot();
  for(auto cycle : range(11)) {
    tick<true>(); sprite.patternFetch(cycle + 23); sprite.scan();
  }

  layerA.begin();
  layerB.begin();
  window.begin();

  tick<true>(); layers.hscrollFetch();
  tick<true>(); sprite.patternFetch(34); sprite.scan();
  tick<true>(); sprite.patternFetch(35); sprite.scan();
  tick<true>(); sprite.patternFetch(36); sprite.scan();
  tick<true>(); sprite.patternFetch(37); sprite.scan();

  layers.vscrollFetch(-1);
  layerA.attributesFetch();
  layerB.attributesFetch();
  window.attributesFetch(-1);

  tick<true>(); layerA.mappingFetch(-1);
  if(!displayEnable()) {
    tick<true,true>(); //refresh
  } else {
    tick<true>(); sprite.patternFetch(38); sprite.scan();
  }
  tick<true>(); layerA.patternFetch( 0); sprite.scan();
  tick<true>(); layerA.patternFetch( 1); sprite.scan();
  tick<true>(); layerB.mappingFetch(-1);
  tick<true>(); sprite.patternFetch(39); sprite.scan();
  tick<true>(); layerB.patternFetch( 0); sprite.scan();
  tick<true>(); layerB.patternFetch( 1); sprite.scan();
}

template<bool _h40, bool _pixels> auto VDP::blocks() -> void {
  bool top = vcounter() == state.topline;
  dac.fillLeftBorder();
  for(auto block : range(_h40 ? 20 : 16)) {
    layers.vscrollFetch(block);
    layerA.attributesFetch();
    layerB.attributesFetch();
    window.attributesFetch(block);
    tick<_h40>(); layerA.mappingFetch(block);
    if((block & 3) == 3) {
      tick<_h40,true>(); //refresh
    } else {
      tick<_h40>(); slot();
    }
    bool den = displayEnable();
    tick<_h40>(); layerA.patternFetch(block * 2 + 2);
    tick<_h40>(); layerA.patternFetch(block * 2 + 3);
    tick<_h40>(); layerB.mappingFetch(block);
    tick<_h40>(); sprite.mappingFetch(block);
    tick<_h40>(); layerB.patternFetch(block * 2 + 2);
    tick<_h40>(); layerB.patternFetch(block * 2 + 3);

    if(_pixels) {
      if(!den || top) {
        for(auto pixel: range(16)) dac.pixel<_h40, false>(block * 16 + pixel);
      } else {
        for(auto pixel: range(16)) dac.pixel<_h40, true>(block * 16 + pixel);
      }
    }
  }
  dac.fillRightBorder();
}

