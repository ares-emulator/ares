auto ANTIC::scanline() -> void {
  dma.beginScanline();
  if(counter.scanline == 248) interrupt.schedule(0x40);
  displayList.beginScanline();

  for(counter.machineCycle = 0; counter.machineCycle < Timing::MachineCyclesPerScanline; counter.machineCycle++) {
    clock();
  }

  finishScanline();
}

auto ANTIC::clock() -> void {
  displayList.clock();
  interrupt.clock();
  wsync.clock();

  DMA::Requests requests = {};
  dma.queuePlayerMissile(requests);
  displayList.queueDMA(requests);
  playfield.queueDMA(requests);

  dma.clockRefresh();
  dma.arbitrate(requests);
  clockRegisterPipelines();
}

auto ANTIC::step(u32 colorClocks) -> void {
  auto firstColorClock = (u16)counter.machineCycle * Timing::ColorClocksPerMachineCycle;
  for(u32 phase = 0; phase < colorClocks; phase++) {
    gtia.clock(playfield.clockAN(firstColorClock + phase));
  }
  Thread::step(colorClocks);
  Thread::synchronize(cpu);
}

auto ANTIC::clockRegisterPipelines() -> void {
  // AHRM 4.2 and 4.13: CHBASE and P/M DMA enable reach their consumers two
  // machine cycles after the CPU write.
  io.chbasePipeline[1] = io.chbasePipeline[0];
  io.chbasePipeline[0] = io.chbase;
  io.playerMissileDMA[1] = io.playerMissileDMA[0];
  io.playerMissileDMA[0] = (u8)io.dmactl >> 2 & 3;

  // AHRM 4.6: cycle 1 uses the display-list DMA state sampled on cycle 113.
  if(counter.machineCycle == 113) {
    displayList.dmaEnabled = io.dmactl.bit(5);
  }
}

auto ANTIC::finishScanline() -> void {
  displayList.finishScanline();
  if(++counter.scanline == Timing::ScanlinesPerFrame) {
    counter.scanline = 0;
    frame();
  }
  counter.machineCycle = 0;
}

auto ANTIC::frame() -> void {
  gtia.frame();
  scheduler.exit(Event::Frame);
}
