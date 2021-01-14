auto CPU::serialize(serializer& s) -> void {
  SM83::serialize(s);
  Thread::serialize(s);

  s(wram);
  s(hram);

  s(status.clock);
  s(status.interruptLatch);
  s(status.hblankPending);

  s(status.joyp);
  s(status.p14);
  s(status.p15);

  s(status.serialData);
  s(status.serialBits);

  s(status.serialClock);
  s(status.serialSpeed);
  s(status.serialTransfer);

  s(status.div);
  s(status.tima);
  s(status.tma);
  s(status.timerClock);
  s(status.timerEnable);

  s(status.interruptFlag);

  s(status.speedSwitch);
  s(status.speedDouble);

  s(status.dmaSource);
  s(status.dmaTarget);
  s(status.dmaLength);
  s(status.hdmaActive);

  s(status.ff6c);

  s(status.wramBank);

  s(status.ff72);
  s(status.ff73);
  s(status.ff74);
  s(status.ff75);

  s(status.interruptEnable);
}
