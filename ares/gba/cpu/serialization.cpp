auto CPU::serialize(serializer& s) -> void {
  ARM7TDMI::serialize(s);
  Thread::serialize(s);

  s(iwram);
  s(ewram);

  s(dmabus.data);

  for(auto& dma : this->dma) {
    s(dma.id);
    s(dma.active);
    s(dma.waiting);
    s(dma.targetMode);
    s(dma.sourceMode);
    s(dma.repeat);
    s(dma.size);
    s(dma.drq);
    s(dma.timingMode);
    s(dma.irq);
    s(dma.enable);
    s(dma.source.data);
    s(dma.source.mask);
    s(dma.target.data);
    s(dma.target.mask);
    s(dma.length.data);
    s(dma.length.mask);
    s(dma.latch.source.data);
    s(dma.latch.source.mask);
    s(dma.latch.target.data);
    s(dma.latch.target.mask);
    s(dma.latch.length.data);
    s(dma.latch.length.mask);
  }

  for(auto& timer : this->timer) {
    s(timer.id);
    s(timer.pending);
    s(timer.period);
    s(timer.reload);
    s(timer.frequency);
    s(timer.cascade);
    s(timer.irq);
    s(timer.enable);
  }

  s(serial.shiftClockSelect);
  s(serial.shiftClockFrequency);
  s(serial.transferEnableReceive);
  s(serial.transferEnableSend);
  s(serial.startBit);
  s(serial.transferLength);
  s(serial.irqEnable);
  for(auto& value : serial.data) s(value);
  s(serial.data8);

  s(keypad.enable);
  s(keypad.condition);
  for(auto& flag : keypad.flag) s(flag);

  s(joybus.sc);
  s(joybus.sd);
  s(joybus.si);
  s(joybus.so);
  s(joybus.scMode);
  s(joybus.sdMode);
  s(joybus.siMode);
  s(joybus.soMode);
  s(joybus.siIRQEnable);
  s(joybus.mode);
  s(joybus.resetSignal);
  s(joybus.receiveComplete);
  s(joybus.sendComplete);
  s(joybus.resetIRQEnable);
  s(joybus.receive);
  s(joybus.transmit);
  s(joybus.receiveFlag);
  s(joybus.sendFlag);
  s(joybus.generalFlag);

  s(irq.ime);
  s(irq.enable);
  s(irq.flag);

  for(auto& flag : wait.nwait) s(flag);
  for(auto& flag : wait.swait) s(flag);
  s(wait.phi);
  s(wait.prefetch);
  s(wait.gameType);

  s(memory.biosSwap);
  s(memory.unknown1);
  s(memory.ewram);
  s(memory.ewramWait);
  s(memory.unknown2);

  s(prefetch.slot);
  s(prefetch.addr);
  s(prefetch.load);
  s(prefetch.wait);

  s(context.clock);
  s(context.halted);
  s(context.stopped);
  s(context.booted);
  s(context.dmaActive);
}
