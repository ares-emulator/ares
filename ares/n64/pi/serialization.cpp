auto PI::serialize(serializer& s) -> void {
  s(bb_rtc);

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

  s(bb_gpio.power.lineIn);
  s(bb_gpio.power.lineOut);
  s(bb_gpio.power.outputEnable);
  s(bb_gpio.led.lineIn);
  s(bb_gpio.led.lineOut);
  s(bb_gpio.led.outputEnable);
  s(bb_gpio.rtc_clock.lineIn);
  s(bb_gpio.rtc_clock.lineOut);
  s(bb_gpio.rtc_clock.outputEnable);
  s(bb_gpio.rtc_data.lineIn);
  s(bb_gpio.rtc_data.lineOut);
  s(bb_gpio.rtc_data.outputEnable);

  s(bb_allowed.buf);
  s(bb_allowed.flash);
  s(bb_allowed.atb);
  s(bb_allowed.aes);
  s(bb_allowed.dma);
  s(bb_allowed.gpio);
  s(bb_allowed.ide);
  s(bb_allowed.err);

  s(bb_ide[0].data);
  s(bb_ide[0].dirty);
  s(bb_ide[1].data);
  s(bb_ide[1].dirty);
  s(bb_ide[2].data);
  s(bb_ide[2].dirty);
  s(bb_ide[3].data);
  s(bb_ide[3].dirty);

  s(bb_nand.buffer);
  s(bb_nand.io.busy);
  s(bb_nand.io.sbErr);
  s(bb_nand.io.dbErr);
  s(bb_nand.io.intrDone);
  s(bb_nand.io.unk24_29);
  s(bb_nand.io.command);
  s(bb_nand.io.unk15);
  s(bb_nand.io.bufferSel);
  s(bb_nand.io.deviceSel);
  s(bb_nand.io.ecc);
  s(bb_nand.io.multiCycle);
  s(bb_nand.io.xferLen);
  s(bb_nand.io.pageNumber);
}
