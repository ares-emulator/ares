auto CPU::serialize(serializer& s) -> void {
  TLCS900H::serialize(s);
  Thread::serialize(s);

  s(ram);

  s(interrupts.vector);
  s(interrupts.priority);

  s(nmi);
  s(intwd);
  s(int0);
  s(int4);
  s(int5);
  s(int6);
  s(int7);
  s(intt0);
  s(intt1);
  s(intt2);
  s(intt3);
  s(inttr4);
  s(inttr5);
  s(inttr6);
  s(inttr7);
  s(intrx0);
  s(inttx0);
  s(intrx1);
  s(inttx1);
  s(intad);
  s(inttc0);
  s(inttc1);
  s(inttc2);
  s(inttc3);

  s(dma0.vector);
  s(dma1.vector);
  s(dma2.vector);
  s(dma3.vector);

  s(p10.latch); s(p10.flow);
  s(p11.latch); s(p11.flow);
  s(p12.latch); s(p12.flow);
  s(p13.latch); s(p13.flow);
  s(p14.latch); s(p14.flow);
  s(p15.latch); s(p15.flow);
  s(p16.latch); s(p16.flow);
  s(p17.latch); s(p17.flow);

  s(p20.latch); s(p20.mode);
  s(p21.latch); s(p21.mode);
  s(p22.latch); s(p22.mode);
  s(p23.latch); s(p23.mode);
  s(p24.latch); s(p24.mode);
  s(p25.latch); s(p25.mode);
  s(p26.latch); s(p26.mode);
  s(p27.latch); s(p27.mode);

  s(p52.latch); s(p52.flow); s(p52.mode);
  s(p53.latch); s(p53.flow); s(p53.mode);
  s(p54.latch); s(p54.flow); s(p54.mode);
  s(p55.latch); s(p55.flow); s(p55.mode);

  s(p60.latch); s(p60.mode);
  s(p61.latch); s(p61.mode);
  s(p62.latch); s(p62.mode);
  s(p63.latch); s(p63.mode);
  s(p64.latch); s(p64.mode);
  s(p65.latch); s(p65.mode);

  s(p70.latch); s(p70.flow); s(p70.mode);
  s(p71.latch); s(p71.flow); s(p71.mode);
  s(p72.latch); s(p72.flow); s(p72.mode);
  s(p73.latch); s(p73.flow); s(p73.mode);
  s(p74.latch); s(p74.flow); s(p74.mode);
  s(p75.latch); s(p75.flow); s(p75.mode);
  s(p76.latch); s(p76.flow); s(p76.mode);
  s(p77.latch); s(p77.flow); s(p77.mode);

  s(p80.latch); s(p80.flow); s(p80.mode); s(p80.drain);
  s(p81.latch); s(p81.flow);
  s(p82.latch); s(p82.flow); s(p82.mode);
  s(p83.latch); s(p83.flow); s(p83.mode); s(p83.drain);
  s(p84.latch); s(p84.flow);
  s(p85.latch); s(p85.flow); s(p85.mode);

  s(p90.latch);
  s(p91.latch);
  s(p92.latch);
  s(p93.latch);

  s(pa0.latch); s(pa0.flow);
  s(pa1.latch); s(pa1.flow);
  s(pa2.latch); s(pa2.flow); s(pa2.mode);
  s(pa3.latch); s(pa3.flow); s(pa3.mode);

  s(pb0.latch); s(pb0.flow);
  s(pb1.latch); s(pb1.flow);
  s(pb2.latch); s(pb2.flow); s(pb2.mode);
  s(pb3.latch); s(pb3.flow); s(pb3.mode);
  s(pb4.latch); s(pb4.flow);
  s(pb5.latch); s(pb5.flow);
  s(pb6.latch); s(pb6.flow); s(pb6.mode);
  s(pb7.latch); s(pb7.flow);

  s(prescaler.enable);
  s(prescaler.counter);

  s(ti0.latch);
  s(ti4.latch);
  s(ti5.latch);
  s(ti6.latch);
  s(ti7.latch);

  s(to1.latch);
  s(to3.latch);
  s(to4.latch);
  s(to5.latch);
  s(to6.latch);
  s(to7.latch);

  s(t0.enable);
  s(t0.mode);
  s(t0.counter);
  s(t0.compare);

  s(t1.enable);
  s(t1.mode);
  s(t1.counter);
  s(t1.compare);

  s(ff1.source);
  s(ff1.invert);
  s(ff1.output);

  s(t01.mode);
  s(t01.pwm);
  s(t01.buffer.enable);
  s(t01.buffer.compare);

  s(t2.enable);
  s(t2.mode);
  s(t2.counter);
  s(t2.compare);

  s(t3.enable);
  s(t3.mode);
  s(t3.counter);
  s(t3.compare);

  s(ff3.source);
  s(ff3.invert);
  s(ff3.output);

  s(t23.mode);
  s(t23.pwm);
  s(t23.buffer.enable);
  s(t23.buffer.compare);

  s(ff4.flipOnCompare4);
  s(ff4.flipOnCompare5);
  s(ff4.flipOnCapture1);
  s(ff4.flipOnCapture2);
  s(ff4.output);

  s(ff5.flipOnCompare5);
  s(ff5.flipOnCapture2);
  s(ff5.output);

  s(t4.enable);
  s(t4.mode);
  s(t4.captureMode);
  s(t4.clearOnCompare5);
  s(t4.counter);
  s(t4.compare4);
  s(t4.compare5);
  s(t4.capture1);
  s(t4.capture2);
  s(t4.buffer.enable);
  s(t4.buffer.compare);

  s(ff6.flipOnCompare6);
  s(ff6.flipOnCompare7);
  s(ff6.flipOnCapture3);
  s(ff6.flipOnCapture4);

  s(t5.enable);
  s(t5.mode);
  s(t5.captureMode);
  s(t5.clearOnCompare7);
  s(t5.counter);
  s(t5.compare6);
  s(t5.compare7);
  s(t5.capture3);
  s(t5.capture4);
  s(t5.buffer.enable);
  s(t5.buffer.compare);

  s(pg0.shiftTrigger);
  s(pg0.shiftAlternateRegister);
  s(pg0.patternGenerationOutput);
  s(pg0.triggerInputEnable);
  s(pg0.excitationMode);
  s(pg0.rotatingDirection);
  s(pg0.writeMode);

  s(pg1.shiftTrigger);
  s(pg1.shiftAlternateRegister);
  s(pg1.patternGenerationOutput);
  s(pg1.triggerInputEnable);
  s(pg1.excitationMode);
  s(pg1.rotatingDirection);
  s(pg1.writeMode);

  s(sc0.buffer);
  s(sc0.baudRateDividend);
  s(sc0.baudRateDivider);
  s(sc0.inputClock);
  s(sc0.clockEdge);
  s(sc0.framingError);
  s(sc0.parityError);
  s(sc0.overrunError);
  s(sc0.parityAddition);
  s(sc0.parity);
  s(sc0.receiveBit8);
  s(sc0.clock);
  s(sc0.mode);
  s(sc0.wakeUp);
  s(sc0.receiving);
  s(sc0.handshake);
  s(sc0.transferBit8);

  s(sc1.buffer);
  s(sc1.baudRateDividend);
  s(sc1.baudRateDivider);
  s(sc1.inputClock);
  s(sc1.clockEdge);
  s(sc1.framingError);
  s(sc1.parityError);
  s(sc1.overrunError);
  s(sc1.parityAddition);
  s(sc1.parity);
  s(sc1.receiveBit8);
  s(sc1.clock);
  s(sc1.mode);
  s(sc1.wakeUp);
  s(sc1.receiving);
  s(sc1.handshake);
  s(sc1.transferBit8);

  s(adc.counter);
  s(adc.channel);
  s(adc.speed);
  s(adc.scan);
  s(adc.repeat);
  s(adc.busy);
  s(adc.end);
  s(adc.result);

  s(rtc.counter);
  s(rtc.enable);
  s(rtc.second);
  s(rtc.minute);
  s(rtc.hour);
  s(rtc.weekday);
  s(rtc.day);
  s(rtc.month);
  s(rtc.year);

  s(watchdog.counter);
  s(watchdog.enable);
  s(watchdog.drive);
  s(watchdog.reset);
  s(watchdog.standby);
  s(watchdog.warmup);
  s(watchdog.frequency);

  s(dram.refreshCycle);
  s(dram.refreshCycleWidth);
  s(dram.refreshCycleInsertion);
  s(dram.dummyCycle);
  s(dram.memoryAccessEnable);
  s(dram.multiplexAddressLength);
  s(dram.multiplexAddressEnable);
  s(dram.memoryAccessSpeed);
  s(dram.busReleaseMode);
  s(dram.selfRefresh);

  s(io.width);
  s(io.timing);

  s(rom.width);
  s(rom.timing);

  s(cram.width);
  s(cram.timing);

  s(aram.width);
  s(aram.timing);

  s(vram.width);
  s(vram.timing);

  s(cs0.width);
  s(cs0.timing);
  s(cs0.enable);
  s(cs0.address);
  s(cs0.mask);

  s(cs1.width);
  s(cs1.timing);
  s(cs1.enable);
  s(cs1.address);
  s(cs1.mask);

  s(cs2.width);
  s(cs2.timing);
  s(cs2.enable);
  s(cs2.address);
  s(cs2.mask);
  s(cs2.mode);

  s(cs3.width);
  s(cs3.timing);
  s(cs3.enable);
  s(cs3.address);
  s(cs3.mask);
  s(cs3.cas);

  s(csx.width);
  s(csx.timing);

  s(clock.rate);

  s(misc.p5);
  s(misc.rtsDisable);

  s(unknown.b4);
  s(unknown.b5);
  s(unknown.b6);
  s(unknown.b7);
}

auto CPU::Interrupt::serialize(serializer& s) -> void {
  s(vector);
  s(dmaAllowed);
  s(enable);
  s(maskable);
  s(priority);
  s(line);
  s(pending);
  s(level.high);
  s(level.low);
  s(edge.rising);
  s(edge.falling);
}
