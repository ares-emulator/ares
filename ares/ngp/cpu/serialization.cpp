auto CPU::serialize(serializer& s) -> void {
  TLCS900H::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);

  s.integer(interrupts.vector);
  s.integer(interrupts.priority);

  nmi.serialize(s);
  intwd.serialize(s);
  int0.serialize(s);
  int4.serialize(s);
  int5.serialize(s);
  int6.serialize(s);
  int7.serialize(s);
  intt0.serialize(s);
  intt1.serialize(s);
  intt2.serialize(s);
  intt3.serialize(s);
  inttr4.serialize(s);
  inttr5.serialize(s);
  inttr6.serialize(s);
  inttr7.serialize(s);
  intrx0.serialize(s);
  inttx0.serialize(s);
  intrx1.serialize(s);
  inttx1.serialize(s);
  intad.serialize(s);
  inttc0.serialize(s);
  inttc1.serialize(s);
  inttc2.serialize(s);
  inttc3.serialize(s);

  s.integer(dma0.vector);
  s.integer(dma1.vector);
  s.integer(dma2.vector);
  s.integer(dma3.vector);

  s.integer(p10.latch); s.integer(p10.flow);
  s.integer(p11.latch); s.integer(p11.flow);
  s.integer(p12.latch); s.integer(p12.flow);
  s.integer(p13.latch); s.integer(p13.flow);
  s.integer(p14.latch); s.integer(p14.flow);
  s.integer(p15.latch); s.integer(p15.flow);
  s.integer(p16.latch); s.integer(p16.flow);
  s.integer(p17.latch); s.integer(p17.flow);

  s.integer(p20.latch); s.integer(p20.mode);
  s.integer(p21.latch); s.integer(p21.mode);
  s.integer(p22.latch); s.integer(p22.mode);
  s.integer(p23.latch); s.integer(p23.mode);
  s.integer(p24.latch); s.integer(p24.mode);
  s.integer(p25.latch); s.integer(p25.mode);
  s.integer(p26.latch); s.integer(p26.mode);
  s.integer(p27.latch); s.integer(p27.mode);

  s.integer(p52.latch); s.integer(p52.flow); s.integer(p52.mode);
  s.integer(p53.latch); s.integer(p53.flow); s.integer(p53.mode);
  s.integer(p54.latch); s.integer(p54.flow); s.integer(p54.mode);
  s.integer(p55.latch); s.integer(p55.flow); s.integer(p55.mode);

  s.integer(p60.latch); s.integer(p60.mode);
  s.integer(p61.latch); s.integer(p61.mode);
  s.integer(p62.latch); s.integer(p62.mode);
  s.integer(p63.latch); s.integer(p63.mode);
  s.integer(p64.latch); s.integer(p64.mode);
  s.integer(p65.latch); s.integer(p65.mode);

  s.integer(p70.latch); s.integer(p70.flow); s.integer(p70.mode);
  s.integer(p71.latch); s.integer(p71.flow); s.integer(p71.mode);
  s.integer(p72.latch); s.integer(p72.flow); s.integer(p72.mode);
  s.integer(p73.latch); s.integer(p73.flow); s.integer(p73.mode);
  s.integer(p74.latch); s.integer(p74.flow); s.integer(p74.mode);
  s.integer(p75.latch); s.integer(p75.flow); s.integer(p75.mode);
  s.integer(p76.latch); s.integer(p76.flow); s.integer(p76.mode);
  s.integer(p77.latch); s.integer(p77.flow); s.integer(p77.mode);

  s.integer(p80.latch); s.integer(p80.flow); s.integer(p80.mode);
  s.integer(p81.latch); s.integer(p81.flow);
  s.integer(p82.latch); s.integer(p82.flow); s.integer(p82.mode);
  s.integer(p83.latch); s.integer(p83.flow); s.integer(p83.mode);
  s.integer(p84.latch); s.integer(p84.flow);
  s.integer(p85.latch); s.integer(p85.flow); s.integer(p85.mode);

  s.integer(p90.latch);
  s.integer(p91.latch);
  s.integer(p92.latch);
  s.integer(p93.latch);

  s.integer(pa0.latch); s.integer(pa0.flow);
  s.integer(pa1.latch); s.integer(pa1.flow);
  s.integer(pa2.latch); s.integer(pa2.flow); s.integer(pa2.mode);
  s.integer(pa3.latch); s.integer(pa3.flow); s.integer(pa3.mode);

  s.integer(pb0.latch); s.integer(pb0.flow);
  s.integer(pb1.latch); s.integer(pb1.flow);
  s.integer(pb2.latch); s.integer(pb2.flow); s.integer(pb2.mode);
  s.integer(pb3.latch); s.integer(pb3.flow); s.integer(pb3.mode);
  s.integer(pb4.latch); s.integer(pb4.flow);
  s.integer(pb5.latch); s.integer(pb5.flow);
  s.integer(pb6.latch); s.integer(pb6.flow); s.integer(pb6.mode);
  s.integer(pb7.latch); s.integer(pb7.flow);

  s.integer(prescaler.enable);
  s.integer(prescaler.counter);

  s.integer(ti0.latch);
  s.integer(ti4.latch);
  s.integer(ti5.latch);
  s.integer(ti6.latch);
  s.integer(ti7.latch);

  s.integer(to1.latch);
  s.integer(to3.latch);
  s.integer(to4.latch);
  s.integer(to5.latch);
  s.integer(to6.latch);
  s.integer(to7.latch);

  s.integer(t0.enable);
  s.integer(t0.mode);
  s.integer(t0.counter);
  s.integer(t0.compare);

  s.integer(t1.enable);
  s.integer(t1.mode);
  s.integer(t1.counter);
  s.integer(t1.compare);

  s.integer(ff1.source);
  s.integer(ff1.invert);
  s.integer(ff1.output);

  s.integer(t01.mode);
  s.integer(t01.pwm);
  s.integer(t01.buffer.enable);
  s.integer(t01.buffer.compare);

  s.integer(t2.enable);
  s.integer(t2.mode);
  s.integer(t2.counter);
  s.integer(t2.compare);

  s.integer(t3.enable);
  s.integer(t3.mode);
  s.integer(t3.counter);
  s.integer(t3.compare);

  s.integer(ff3.source);
  s.integer(ff3.invert);
  s.integer(ff3.output);

  s.integer(t23.mode);
  s.integer(t23.pwm);
  s.integer(t23.buffer.enable);
  s.integer(t23.buffer.compare);

  s.integer(ff4.flipOnCompare4);
  s.integer(ff4.flipOnCompare5);
  s.integer(ff4.flipOnCapture1);
  s.integer(ff4.flipOnCapture2);
  s.integer(ff4.output);

  s.integer(ff5.flipOnCompare5);
  s.integer(ff5.flipOnCapture2);
  s.integer(ff5.output);

  s.integer(t4.enable);
  s.integer(t4.mode);
  s.integer(t4.captureMode);
  s.integer(t4.clearOnCompare5);
  s.integer(t4.counter);
  s.integer(t4.compare4);
  s.integer(t4.compare5);
  s.integer(t4.capture1);
  s.integer(t4.capture2);
  s.integer(t4.buffer.enable);
  s.integer(t4.buffer.compare);

  s.integer(ff6.flipOnCompare6);
  s.integer(ff6.flipOnCompare7);
  s.integer(ff6.flipOnCapture3);
  s.integer(ff6.flipOnCapture4);

  s.integer(t5.enable);
  s.integer(t5.mode);
  s.integer(t5.captureMode);
  s.integer(t5.clearOnCompare7);
  s.integer(t5.counter);
  s.integer(t5.compare6);
  s.integer(t5.compare7);
  s.integer(t5.capture3);
  s.integer(t5.capture4);
  s.integer(t5.buffer.enable);
  s.integer(t5.buffer.compare);

  s.integer(adc.counter);
  s.integer(adc.channel);
  s.integer(adc.speed);
  s.integer(adc.scan);
  s.integer(adc.repeat);
  s.integer(adc.busy);
  s.integer(adc.end);
  s.integer(adc.result[0]);
  s.integer(adc.result[1]);
  s.integer(adc.result[2]);
  s.integer(adc.result[3]);

  s.integer(rtc.counter);
  s.integer(rtc.enable);
  s.integer(rtc.second);
  s.integer(rtc.minute);
  s.integer(rtc.hour);
  s.integer(rtc.weekday);
  s.integer(rtc.day);
  s.integer(rtc.month);
  s.integer(rtc.year);

  s.integer(watchdog.counter);
  s.integer(watchdog.enable);
  s.integer(watchdog.drive);
  s.integer(watchdog.reset);
  s.integer(watchdog.standby);
  s.integer(watchdog.warmup);
  s.integer(watchdog.frequency);

  s.integer(io.width);
  s.integer(io.timing);

  s.integer(rom.width);
  s.integer(rom.timing);

  s.integer(cram.width);
  s.integer(cram.timing);

  s.integer(aram.width);
  s.integer(aram.timing);

  s.integer(vram.width);
  s.integer(vram.timing);

  s.integer(cs0.width);
  s.integer(cs0.timing);
  s.integer(cs0.enable);
  s.integer(cs0.address);
  s.integer(cs0.mask);

  s.integer(cs1.width);
  s.integer(cs1.timing);
  s.integer(cs1.enable);
  s.integer(cs1.address);
  s.integer(cs1.mask);

  s.integer(cs2.width);
  s.integer(cs2.timing);
  s.integer(cs2.enable);
  s.integer(cs2.address);
  s.integer(cs2.mask);
  s.integer(cs2.mode);

  s.integer(cs3.width);
  s.integer(cs3.timing);
  s.integer(cs3.enable);
  s.integer(cs3.address);
  s.integer(cs3.mask);
  s.integer(cs3.cas);

  s.integer(csx.width);
  s.integer(csx.timing);

  s.integer(clock.rate);

  s.integer(misc.p5);
  s.integer(misc.rtsDisable);
  s.integer(misc.b4);
  s.integer(misc.b5);
}

auto CPU::Interrupt::serialize(serializer& s) -> void {
  s.integer(vector);
  s.integer(dmaAllowed);
  s.integer(enable);
  s.integer(maskable);
  s.integer(priority);
  s.integer(line);
  s.integer(pending);
  s.integer(level.high);
  s.integer(level.low);
  s.integer(edge.rising);
  s.integer(edge.falling);
}
