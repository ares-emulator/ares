auto PI::serialize(serializer& s) -> void {
  s(io.dmaBusy);
  s(io.ioBusy);
  s(io.error);
  s(io.interrupt);
  s(io.dramAddress);
  s(io.pbusAddress);
  s(io.readLength);
  s(io.writeLength);
  s(io.busLatch);
  s(io.originPc);

  s(bsd1.latency);
  s(bsd1.pulseWidth);
  s(bsd1.pageSize);
  s(bsd1.releaseDuration);

  s(bsd2.latency);
  s(bsd2.pulseWidth);
  s(bsd2.pageSize);
  s(bsd2.releaseDuration);

  s(bb_gpio.power.data);
  s(bb_gpio.power.mask);
  s(bb_gpio.led.data);
  s(bb_gpio.led.mask);
  s(bb_gpio.rtc_clock.data);
  s(bb_gpio.rtc_clock.mask);
  s(bb_gpio.rtc_data.data);
  s(bb_gpio.rtc_data.mask);

  s(bb_allowed.buf);
  s(bb_allowed.flash);
  s(bb_allowed.atb);
  s(bb_allowed.aes);
  s(bb_allowed.dma);
  s(bb_allowed.gpio);
  s(bb_allowed.ide);
  s(bb_allowed.err);

  s(bb_ide);
}
