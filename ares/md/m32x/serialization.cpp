auto M32X::serialize(serializer& s) -> void {
  s(sdram);
  s(shm);
  s(shs);
  s(vdp);
  s(pwm);

  s(io.vectorLevel4);
  s(io.adapterEnable);
  s(io.adapterReset);
  s(io.resetEnable);
  s(io.romBank);
  s(io.cartridgeMode);
  s(io.hperiod);
  s(io.hcounter);
  s(io.hintVblank);
  s(dreq.vram);
  s(dreq.dma);
  s(dreq.active);
  s(dreq.source);
  s(dreq.target);
  s(dreq.length);
  s(dreq.fifo);
  s(communication);
}

auto M32X::SH7604::serialize(serializer& s) -> void {
  Thread::serialize(s);
  SH2::serialize(s);
  s(irq.pwm.enable);
  s(irq.pwm.active);
  s(irq.cmd.enable);
  s(irq.cmd.active);
  s(irq.hint.enable);
  s(irq.hint.active);
  s(irq.vint.enable);
  s(irq.vint.active);
  s(irq.vres.enable);
  s(irq.vres.active);
}

auto M32X::VDP::serialize(serializer& s) -> void {
  s(dram);
  s(cram);
  s(mode);
  s(lines);
  s(priority);
  s(dotshift);
  s(autofillLength);
  s(autofillAddress);
  s(autofillData);
  s(framebufferAccess);
  s(framebufferActive);
  s(framebufferSelect);
  s(hblank);
  s(vblank);
  selectFramebuffer(framebufferSelect);
}

auto M32X::PWM::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(lmode);
  s(rmode);
  s(mono);
  s(dreqIRQ);
  s(timer);
  s(cycle);
  s(periods);
  s(counter);
  s(lsample);
  s(rsample);
  s(lfifo);
  s(rfifo);
}
