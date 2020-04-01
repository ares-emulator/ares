auto CPU::serialize(serializer& s) -> void {
  SM83::serialize(s);
  Thread::serialize(s);

  wram.serialize(s);
  hram.serialize(s);

  s.integer(status.clock);
  s.integer(status.interruptLatch);
  s.integer(status.hblankPending);

  s.integer(status.joyp);
  s.integer(status.p14);
  s.integer(status.p15);

  s.integer(status.serialData);
  s.integer(status.serialBits);

  s.integer(status.serialClock);
  s.integer(status.serialSpeed);
  s.integer(status.serialTransfer);

  s.integer(status.div);
  s.integer(status.tima);
  s.integer(status.tma);
  s.integer(status.timerClock);
  s.integer(status.timerEnable);

  s.integer(status.interruptFlag);

  s.integer(status.speedSwitch);
  s.integer(status.speedDouble);

  s.integer(status.dmaSource);
  s.integer(status.dmaTarget);
  s.integer(status.dmaLength);
  s.integer(status.dmaMode);
  s.integer(status.dmaCompleted);

  s.integer(status.ff6c);

  s.integer(status.wramBank);

  s.integer(status.ff72);
  s.integer(status.ff73);
  s.integer(status.ff74);
  s.integer(status.ff75);

  s.integer(status.interruptEnable);
}
