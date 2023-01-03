auto LSPC::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(vram);
  s(pram);
  s(animation.disable);
  s(animation.speed);
  s(animation.counter);
  s(animation.frame);
  s(timer.interruptEnable);
  s(timer.reloadOnChange);
  s(timer.reloadOnVblank);
  s(timer.reloadOnZero);
  s(timer.reload);
  s(timer.counter);
  s(timer.stopPAL);
  s(irq.powerAcknowledge);
  s(irq.timerAcknowledge);
  s(irq.vblankAcknowledge);
  s(io.hcounter);
  s(io.vcounter);
  s(io.shadow);
  s(io.vramAddress);
  s(io.vramIncrement);
  s(io.pramBank);
  for(u16 x = 0; x < 256; x++)
    for(u16 y = 0; y < 256; y++)
      s(vscale[x][y]);
  for(u16 x = 0; x < 16; x++)
    for(u16 y = 0; y < 16; y++)
      s(hscale[x][y]);
}
