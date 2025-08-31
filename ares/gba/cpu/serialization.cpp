auto CPU::serialize(serializer& s) -> void {
  ARM7TDMI::serialize(s);
  Thread::serialize(s);

  s(iwram);
  s(ewram);

  s(dmac.romBurst);
  s(dmac.active);
  s(dmac.activeChannel);
  s(dmac.stallingCPU);
  s(dmac.writeCycle);
  for(auto& channel : this->dmac.channel) {
    s(channel.id);
    s(channel.active);
    s(channel.waiting);
    s(channel.targetMode);
    s(channel.sourceMode);
    s(channel.repeat);
    s(channel.size);
    s(channel.drq);
    s(channel.timingMode);
    s(channel.irq);
    s(channel.enable);
    s(channel.source.data);
    s(channel.source.mask);
    s(channel.target.data);
    s(channel.target.mask);
    s(channel.length.data);
    s(channel.length.mask);
    s(channel.latch.source.data);
    s(channel.latch.source.mask);
    s(channel.latch.target.data);
    s(channel.latch.target.mask);
    s(channel.latch.length.data);
    s(channel.latch.length.mask);
    s(channel.latch.data);
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
    s(timer.latch.reload);
    s(timer.latch.control);
    s(timer.latch.reloadFlags);
    s(timer.latch.controlFlag);
  }

  s(serial.shiftClockSelect);
  s(serial.shiftClockFrequency);
  s(serial.transferEnableReceive);
  s(serial.transferEnableSend);
  s(serial.startBit);
  s(serial.uartFlags);
  s(serial.mode);
  s(serial.irqEnable);
  for(auto& value : serial.data) s(value);
  s(serial.dataMulti);

  s(keypad.enable);
  s(keypad.condition);
  for(auto& flag : keypad.flag) s(flag);
  s(keypad.conditionMet);

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

  for(auto& flag : irq.ime) s(flag);
  s(irq.synchronizer);
  for(auto& flag : irq.enable) s(flag);
  for(auto& flag : irq.flag) s(flag);

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

  s(openBus.data);
  s(openBus.iwramData);

  s(prefetch.slot);
  s(prefetch.addr);
  s(prefetch.load);
  s(prefetch.wait);
  s(prefetch.stopped);
  s(prefetch.ahead);

  s(context.clock);
  s(context.halted);
  s(context.stopped);
  s(context.booted);
  s(context.romAccess);
  s(context.timerLatched);
  s(context.busLocked);
  s(context.burstActive);
  s(context.hcounter);
}
