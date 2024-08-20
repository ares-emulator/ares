auto CPU::Timer::stepLatch() -> void {
  if(latch.reloadFlags.bit(0)) {
    reload.byte(0) = latch.reload.byte(0);
    latch.reloadFlags.bit(0) = 0;
  }
  
  if(latch.reloadFlags.bit(1)) {
    reload.byte(1) = latch.reload.byte(1);
    latch.reloadFlags.bit(1) = 0;
  }
  
  if(latch.controlFlag) {
    n1 wasEnabled = enable;

    frequency = latch.control.bit(0,1);
    irq       = latch.control.bit(6);
    enable    = latch.control.bit(7);

    if(id != 0) cascade = latch.control.bit(2);

    if(!wasEnabled && enable) {  //0->1 transition
      pending = true;
    }
    latch.controlFlag = 0;
  }
}

inline auto CPU::Timer::reloadLatch() -> void {
  if(pending) {
    period = reload;
    pending = false;
  }
}

inline auto CPU::Timer::run() -> void {
  if(!enable || cascade) return;

  static const u32 mask[] = {0, 63, 255, 1023};
  if((cpu.clock() & mask[frequency]) == 0) step();
}

auto CPU::Timer::step() -> void {
  if(++period == 0) {
    period = reload;

    if(irq) cpu.irq.flag |= CPU::Interrupt::Timer0 << id;

    if(apu.fifo[0].timer == id) cpu.runFIFO(0);
    if(apu.fifo[1].timer == id) cpu.runFIFO(1);

    if(id < 3 && cpu.timer[id + 1].enable && cpu.timer[id + 1].cascade) {
      cpu.timer[id + 1].step();
    }
  }
}

auto CPU::runFIFO(u32 n) -> void {
  synchronize(apu);
  apu.fifo[n].read();
  if(apu.fifo[n].size > 16) return;

  auto& dma = this->dma[1 + n];
  if(dma.enable && dma.timingMode == 3) {
    dma.active = true;
    dma.waiting = 2;
    dma.targetMode = 2;
    dma.size = 1;
    dma.latch.length.data = 4;
  }
}
