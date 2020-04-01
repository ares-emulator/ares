auto CPU::serialize(serializer& s) -> void {
  ARM7TDMI::serialize(s);
  Thread::serialize(s);

  iwram.serialize(s);
  ewram.serialize(s);

  for(auto& dma : this->dma) {
    s.integer(dma.id);
    s.boolean(dma.active);
    s.integer(dma.waiting);
    s.integer(dma.targetMode);
    s.integer(dma.sourceMode);
    s.integer(dma.repeat);
    s.integer(dma.size);
    s.integer(dma.drq);
    s.integer(dma.timingMode);
    s.integer(dma.irq);
    s.integer(dma.enable);
    s.integer(dma.data);
    s.integer(dma.source.data);
    s.integer(dma.source.mask);
    s.integer(dma.target.data);
    s.integer(dma.target.mask);
    s.integer(dma.length.data);
    s.integer(dma.length.mask);
    s.integer(dma.latch.source.data);
    s.integer(dma.latch.source.mask);
    s.integer(dma.latch.target.data);
    s.integer(dma.latch.target.mask);
    s.integer(dma.latch.length.data);
    s.integer(dma.latch.length.mask);
  }

  for(auto& timer : this->timer) {
    s.integer(timer.id);
    s.boolean(timer.pending);
    s.integer(timer.period);
    s.integer(timer.reload);
    s.integer(timer.frequency);
    s.integer(timer.cascade);
    s.integer(timer.irq);
    s.integer(timer.enable);
  }

  s.integer(serial.shiftClockSelect);
  s.integer(serial.shiftClockFrequency);
  s.integer(serial.transferEnableReceive);
  s.integer(serial.transferEnableSend);
  s.integer(serial.startBit);
  s.integer(serial.transferLength);
  s.integer(serial.irqEnable);
  for(auto& value : serial.data) s.integer(value);
  s.integer(serial.data8);

  s.integer(keypad.enable);
  s.integer(keypad.condition);
  for(auto& flag : keypad.flag) s.integer(flag);

  s.integer(joybus.sc);
  s.integer(joybus.sd);
  s.integer(joybus.si);
  s.integer(joybus.so);
  s.integer(joybus.scMode);
  s.integer(joybus.sdMode);
  s.integer(joybus.siMode);
  s.integer(joybus.soMode);
  s.integer(joybus.siIRQEnable);
  s.integer(joybus.mode);
  s.integer(joybus.resetSignal);
  s.integer(joybus.receiveComplete);
  s.integer(joybus.sendComplete);
  s.integer(joybus.resetIRQEnable);
  s.integer(joybus.receive);
  s.integer(joybus.transmit);
  s.integer(joybus.receiveFlag);
  s.integer(joybus.sendFlag);
  s.integer(joybus.generalFlag);

  s.integer(irq.ime);
  s.integer(irq.enable);
  s.integer(irq.flag);

  for(auto& flag : wait.nwait) s.integer(flag);
  for(auto& flag : wait.swait) s.integer(flag);
  s.integer(wait.phi);
  s.integer(wait.prefetch);
  s.integer(wait.gameType);

  s.integer(memory.disable);
  s.integer(memory.unknown1);
  s.integer(memory.ewram);
  s.integer(memory.ewramWait);
  s.integer(memory.unknown2);

  s.array(prefetch.slot);
  s.integer(prefetch.addr);
  s.integer(prefetch.load);
  s.integer(prefetch.wait);

  s.integer(context.clock);
  s.boolean(context.halted);
  s.boolean(context.stopped);
  s.boolean(context.booted);
  s.boolean(context.dmaActive);
}
