auto CPU::readIO(uint8 address) -> uint8 {
//print("CPU::readIO(", hex(address, 2L), ")\n");
  uint8 data = 0x00;

  switch(address) {

  //P1
  case 0x01:
    data.bit(0) = p10;
    data.bit(1) = p11;
    data.bit(2) = p12;
    data.bit(3) = p13;
    data.bit(4) = p14;
    data.bit(5) = p15;
    data.bit(6) = p16;
    data.bit(7) = p17;
    return data;

  //P2
  case 0x06:
    data.bit(0) = p20;
    data.bit(1) = p21;
    data.bit(2) = p22;
    data.bit(3) = p23;
    data.bit(4) = p24;
    data.bit(5) = p25;
    data.bit(6) = p26;
    data.bit(7) = p27;
    return data;

  //P5
  case 0x0d:
    data.bit(0) = misc.p5;
    data.bit(2) = p52;
    data.bit(3) = p53;
    data.bit(4) = p54;
    data.bit(5) = p55;
    return data;

  //P6
  case 0x12:
    data.bit(0) = p60;
    data.bit(1) = p61;
    data.bit(2) = p62;
    data.bit(3) = p63;
    data.bit(4) = p64;
    data.bit(5) = p65;
    return data;

  //P7
  case 0x13:
    data.bit(0) = p70;
    data.bit(1) = p71;
    data.bit(2) = p72;
    data.bit(3) = p73;
    data.bit(4) = p74;
    data.bit(5) = p75;
    data.bit(6) = p76;
    data.bit(7) = p77;
    return data;

  //P8
  case 0x18:
    data.bit(0) = p80;
    data.bit(1) = p81;
    data.bit(2) = p82;
    data.bit(3) = p83;
    data.bit(4) = p84;
    data.bit(5) = p85;
    return data;

  //P9
  case 0x19:
    data.bit(0) = p90;
    data.bit(1) = p91;
    data.bit(2) = p92;
    data.bit(3) = p93;
    return data;

  //PA
  case 0x1e:
    data.bit(0) = pa0;
    data.bit(1) = pa1;
    data.bit(2) = pa2;
    data.bit(3) = pa3;
    return data;

  //PB
  case 0x1f:
    data.bit(0) = pb0;
    data.bit(1) = pb1;
    data.bit(2) = pb2;
    data.bit(3) = pb3;
    data.bit(4) = pb4;
    data.bit(5) = pb5;
    data.bit(6) = pb6;
    data.bit(7) = pb7;
    return data;

  //TRUN
  case 0x20:
    data.bit(0) = t0.enable;
    data.bit(1) = t1.enable;
    data.bit(2) = t2.enable;
    data.bit(3) = t3.enable;
    data.bit(4) = t4.enable;
    data.bit(5) = t5.enable;
    data.bit(7) = prescaler.enable;
    return data;

  //TREG0 (write-only)
  case 0x22: return data;

  //TREG1 (write-only)
  case 0x23: return data;

  //T01MOD
  case 0x24:
    data.bit(0,1) = t0.mode;
    data.bit(2,3) = t1.mode;
    data.bit(4,5) = t01.pwm;
    data.bit(6,7) = t01.mode;
    return data;

  //TFFCR
  case 0x25:
    data.bit(0) = ff1.source;
    data.bit(1) = ff1.invert;
    data.bit(2,3) = 0b11;  //write-only
    data.bit(4) = ff3.source;
    data.bit(5) = ff3.invert;
    data.bit(6,7) = 0b11;  //write-only
    return data;

  //TREG2 (write-only)
  case 0x26: return data;

  //TREG3 (write-only)
  case 0x27: return data;

  //T23MOD
  case 0x28:
    data.bit(0,1) = t2.mode;
    data.bit(2,3) = t3.mode;
    data.bit(4,5) = t23.pwm;
    data.bit(6,7) = t23.mode;
    return data;

  //TRDC
  case 0x29:
    data.bit(0) = t01.buffer.enable;
    data.bit(1) = t23.buffer.enable;
    return data;

  //TREG4 (write-only?)
  case 0x30: return data;
  case 0x31: return data;

  //TREG5 (write-only?)
  case 0x32: return data;
  case 0x33: return data;

  //CAP1
  case 0x34: return t4.capture1.byte(0);
  case 0x35: return t4.capture1.byte(1);

  //CAP2
  case 0x36: return t4.capture2.byte(0);
  case 0x37: return t4.capture2.byte(1);

  //T4MOD
  case 0x38:
    data.bit(0,1) = t4.mode;
    data.bit(2) = t4.clearOnCompare5;
    data.bit(3,4) = t4.captureMode;
    data.bit(5) = 1;
    data.bit(6) = ff5.flipOnCompare5;
    data.bit(7) = ff5.flipOnCapture2;
    return data;

  //T4FFCR
  case 0x39:
    data.bit(0,1) = 0b11;
    data.bit(2) = ff4.flipOnCompare4;
    data.bit(3) = ff4.flipOnCompare5;
    data.bit(4) = ff4.flipOnCapture1;
    data.bit(5) = ff4.flipOnCapture2;
    data.bit(6,7) = 0b11;
    return data;

  //MSAR0
  case 0x3c:
    data.bit(0,7) = cs0.address.bit(16,23);
    return data;

  //MAMR0
  case 0x3d:
    data.bit(0,1) = cs0.mask.bit(8,9);
    data.bit(2,7) = cs0.mask.bit(15,20);
    return data;

  //MSAR1
  case 0x3e:
    data.bit(0,7) = cs1.address.bit(16,23);
    return data;

  //MAMR1
  case 0x3f:
    data.bit(0,1) = cs1.mask.bit(8,9);
    data.bit(2,7) = cs1.mask.bit(16,21);
    return data;

  //TREG6 (write-only?)
  case 0x40: return data;
  case 0x41: return data;

  //TREG7 (write-only?)
  case 0x42: return data;
  case 0x43: return data;

  //CAP3
  case 0x44: return t5.capture3.byte(0);
  case 0x45: return t5.capture4.byte(1);

  //CAP4
  case 0x46: return t5.capture3.byte(0);
  case 0x47: return t5.capture4.byte(1);

  //T5MOD
  case 0x48:
    data.bit(0,1) = t5.mode;
    data.bit(2) = t5.clearOnCompare7;
    data.bit(3,4) = t5.captureMode;
    data.bit(5) = 1;
    return data;

  //T5FFCR
  case 0x49:
    data.bit(0,1) = 0b11;
    data.bit(2) = ff6.flipOnCompare6;
    data.bit(3) = ff6.flipOnCompare7;
    data.bit(4) = ff6.flipOnCapture3;
    data.bit(5) = ff6.flipOnCapture4;
    data.bit(6,7) = 0b11;
    return data;

  //MSAR2
  case 0x5c:
    data.bit(0,7) = cs2.address.bit(16,23);
    return data;

  //MAMR2
  case 0x5d:
    data.bit(0,7) = cs2.mask.bit(15,22);
    return data;

  //MSAR3
  case 0x5e:
    data.bit(0,7) = cs3.address.bit(16,23);
    return data;

  //MAMR3
  case 0x5f:
    data.bit(0,7) = cs3.mask.bit(15,22);
    return data;

  //ADREG04L
  case 0x60:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[0].bit(0,1);
    adc.end = 0;
    return data;

  //ADREG04H
  case 0x61:
    data.bit(0,7) = adc.result[0].bit(2,9);
    adc.end = 0;
    intad.clear();
    return data;

  //ADREG15L
  case 0x62:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[1].bit(0,1);
    adc.end = 0;
    return data;

  //ADREG15H
  case 0x63:
    data.bit(0,7) = adc.result[1].bit(2,9);
    adc.end = 0;
    intad.clear();
    return data;

  //ADREG2L
  case 0x64:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[2].bit(0,1);
    adc.end = 0;
    return data;

  //ADREG2H
  case 0x65:
    data.bit(0,7) = adc.result[2].bit(2,9);
    adc.end = 0;
    intad.clear();
    return data;

  //ADREG3L
  case 0x66:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[3].bit(0,1);
    adc.end = 0;
    return data;

  //ADREG3H
  case 0x67:
    data.bit(0,7) = adc.result[3].bit(2,9);
    adc.end = 0;
    intad.clear();
    return data;

  //ADMOD
  case 0x6d:
    data.bit(0,1) = adc.channel;
    data.bit(2) = 0;  //always reads as zero
    data.bit(3) = adc.speed;
    data.bit(4) = adc.scan;
    data.bit(5) = adc.repeat;
    data.bit(6) = adc.busy;
    data.bit(7) = adc.end;
    return data;

  //WDMOD
  case 0x6e:
    data.bit(0) = watchdog.drive;
    data.bit(1) = watchdog.reset;
    data.bit(2,3) = watchdog.standby;
    data.bit(4) = watchdog.warmup;
    data.bit(5,6) = watchdog.frequency;
    data.bit(7) = watchdog.enable;
    return data;

  //WDCR (write-only)
  case 0x6f: return data;

  //INTE0AD
  case 0x70:
    data.bit(0,2) = 0b000;
    data.bit(3) = int0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = intad.pending;
    return data;

  //INTE45
  case 0x71:
    data.bit(0,2) = 0b000;
    data.bit(3) = int4.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = int5.pending;
    return data;

  //INTE67
  case 0x72:
    data.bit(0,2) = 0b000;
    data.bit(3) = int6.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = int7.pending;
    return data;

  //INTET01
  case 0x73:
    data.bit(0,2) = 0b000;
    data.bit(3) = intt0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = intt1.pending;
    return data;

  //INTET23
  case 0x74:
    data.bit(0,2) = 0b000;
    data.bit(3) = intt2.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = intt3.pending;
    return data;

  //INTET45
  case 0x75:
    data.bit(0,2) = 0b000;
    data.bit(3) = inttr4.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttr5.pending;
    return data;

  //INTET67
  case 0x76:
    data.bit(0,2) = 0b000;
    data.bit(3) = inttr6.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttr7.pending;
    return data;

  //INTE50
  case 0x77:
    data.bit(0,2) = 0b000;
    data.bit(3) = intrx0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttx0.pending;
    return data;

  //INTE51
  case 0x78:
    data.bit(0,2) = 0b000;
    data.bit(3) = intrx1.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttx1.pending;
    return data;

  //INTETC01
  case 0x79:
    data.bit(0,2) = 0b000;
    data.bit(3) = inttc0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttc1.pending;
    return data;

  //INTETC23
  case 0x7a:
    data.bit(0,2) = 0b000;
    data.bit(3) = inttc2.pending;
    data.bit(4,6) = 0b000;
    data.bit(7) = inttc3.pending;
    return data;

  //IIMC
  case 0x7b:
    data.bit(0) = nmi.edge.rising;
    data.bit(1) = int0.level.high;
    data.bit(2) = int0.enable;
    return data;

  case 0x80:
    data.bit(0,2) = clock.rate;  //unconfirmed
    data.bit(3,7) = 0b00000;     //unconfirmed
    return data;

  case 0x90: return rtc.enable;
  case 0x91: return rtc.year;
  case 0x92: return rtc.month;
  case 0x93: return rtc.day;
  case 0x94: return rtc.hour;
  case 0x95: return rtc.minute;
  case 0x96: return rtc.second;
  case 0x97:
    data.bit(0,3) = rtc.weekday;
    data.bit(4,7) = rtc.year & 3;
    return data;

  case 0xb0:
    system.controls.poll();
    data.bit(0) = system.controls.upLatch;
    data.bit(1) = system.controls.downLatch;
    data.bit(2) = system.controls.leftLatch;
    data.bit(3) = system.controls.rightLatch;
    data.bit(4) = system.controls.a->value();
    data.bit(5) = system.controls.b->value();
    data.bit(6) = system.controls.option->value();
    data.bit(7) = system.controls.debugger->value();
    return data;

  case 0xb1:
    //power button polled in CPU::pollPowerButton()
    data.bit(0) = !system.controls.power->value();
    data.bit(1) = 1;  //sub battery (CR2032)
    return data;

  case 0xb2:
    data.bit(0) = misc.rtsDisable;
    return data;

  case 0xb3:
    data.bit(2) = nmi.enable;
    return data;

  case 0xb4: return misc.b4;
  case 0xb5: return misc.b5;

  case 0xbc:
    data = apu.port.data;
    return data;

  }

//print("CPU::readIO(", hex(address, 2L), ")\n");
  return data;
}

auto CPU::writeIO(uint8 address, uint8 data) -> void {
//print("CPU::writeIO(", hex(address, 2L), " = ", hex(data, 2L), ")\n");

  switch(address) {

  //P1
  case 0x01:
    p10 = data.bit(0);
    p11 = data.bit(1);
    p12 = data.bit(2);
    p13 = data.bit(3);
    p14 = data.bit(4);
    p15 = data.bit(5);
    p16 = data.bit(6);
    p17 = data.bit(7);
    return;

  //P1CR
  case 0x04:
    p10.flow = data.bit(0);
    p11.flow = data.bit(1);
    p12.flow = data.bit(2);
    p13.flow = data.bit(3);
    p14.flow = data.bit(4);
    p15.flow = data.bit(5);
    p16.flow = data.bit(6);
    p17.flow = data.bit(7);
    return;

  //P2
  case 0x06:
    p20 = data.bit(0);
    p21 = data.bit(1);
    p22 = data.bit(2);
    p23 = data.bit(3);
    p24 = data.bit(4);
    p25 = data.bit(5);
    p26 = data.bit(6);
    p27 = data.bit(7);
    return;

  //P2FC
  case 0x09:
    p20.mode = data.bit(0);
    p21.mode = data.bit(1);
    p22.mode = data.bit(2);
    p23.mode = data.bit(3);
    p24.mode = data.bit(4);
    p25.mode = data.bit(5);
    p26.mode = data.bit(6);
    p27.mode = data.bit(7);
    return;

  //P5
  case 0x0d:
    misc.p5 = data.bit(0);
    p52 = data.bit(2);
    p53 = data.bit(3);
    p54 = data.bit(4);
    p55 = data.bit(5);
    return;

  //P5CR
  case 0x10:
    p52.flow = data.bit(2);
    p53.flow = data.bit(3);
    p54.flow = data.bit(4);
    p55.flow = data.bit(5);
    return;

  //P5FC
  case 0x11:
    p52.mode = data.bit(2);
    p53.mode = data.bit(3);
    p54.mode = data.bit(4);
    p55.mode = data.bit(5);
    return;

  //P6
  case 0x12:
    p60 = data.bit(0);
    p61 = data.bit(1);
    p62 = data.bit(2);
    p63 = data.bit(3);
    p64 = data.bit(4);
    p65 = data.bit(5);
    return;

  //P7
  case 0x13:
    p70 = data.bit(0);
    p71 = data.bit(1);
    p72 = data.bit(2);
    p73 = data.bit(3);
    p74 = data.bit(4);
    p75 = data.bit(5);
    p76 = data.bit(6);
    p77 = data.bit(7);
    return;

  //P6FC
  case 0x15:
    p60.mode = data.bit(0);
    p61.mode = data.bit(1);
    p62.mode = data.bit(2);
    p63.mode = data.bit(3);
    p64.mode = data.bit(4);
    p65.mode = data.bit(5);
    return;

  //P7CR
  case 0x16:
    p70.flow = data.bit(0);
    p71.flow = data.bit(1);
    p72.flow = data.bit(2);
    p73.flow = data.bit(3);
    p74.flow = data.bit(4);
    p75.flow = data.bit(5);
    p76.flow = data.bit(6);
    p77.flow = data.bit(7);
    return;

  //P7FC
  case 0x17:
    p70.mode = data.bit(0);
    p71.mode = data.bit(1);
    p72.mode = data.bit(2);
    p73.mode = data.bit(3);
    p74.mode = data.bit(4);
    p75.mode = data.bit(5);
    p76.mode = data.bit(6);
    p77.mode = data.bit(7);
    return;

  //P8
  case 0x18:
    p80 = data.bit(0);
    p81 = data.bit(1);
    p82 = data.bit(2);
    p83 = data.bit(3);
    p84 = data.bit(4);
    p85 = data.bit(5);
    return;

  //P8CR
  case 0x1a:
    p80.flow = data.bit(0);
    p81.flow = data.bit(1);
    p82.flow = data.bit(2);
    p83.flow = data.bit(3);
    p84.flow = data.bit(4);
    p85.flow = data.bit(5);
    return;

  //P8FC
  case 0x1b:
    p80.mode = data.bit(0);
    p82.mode = data.bit(2);
    p83.mode = data.bit(3);
    p85.mode = data.bit(5);
    return;

  //PA
  case 0x1e:
    pa0 = data.bit(0);
    pa1 = data.bit(1);
    if(pa2.mode == 0) pa2 = data.bit(2);
    if(pa2.mode == 0) pa3 = data.bit(3);
    return;

  //PB
  case 0x1f:
    pb0 = data.bit(0);
    pb1 = data.bit(1);
    return;

  //TRUN
  case 0x20:
    if(t0.enable && !data.bit(0)) t0.disable();
    if(t1.enable && !data.bit(1)) t1.disable();
    if(t2.enable && !data.bit(2)) t2.disable();
    if(t3.enable && !data.bit(3)) t3.disable();
    if(t4.enable && !data.bit(4)) t4.disable();
    if(t5.enable && !data.bit(5)) t5.disable();
    t0.enable = data.bit(0);
    t1.enable = data.bit(1);
    t2.enable = data.bit(2);
    t3.enable = data.bit(3);
    t4.enable = data.bit(4);
    t5.enable = data.bit(5);
    prescaler.enable = data.bit(7);
    if(!prescaler.enable) prescaler.counter = 0;
    return;

  //TREG0
  case 0x22:
    if(!t01.buffer.enable) t0.compare = data;
    t01.buffer.compare = data;
    return;

  //TREG1
  case 0x23:
    t1.compare = data;
    return;

  //T01MOD
  case 0x24:
    t0.mode  = data.bit(0,1);
    t1.mode  = data.bit(2,3);
    t01.pwm  = data.bit(4,5);
    t01.mode = data.bit(6,7);
    return;

  //TFFCR
  case 0x25:
    ff1.source = data.bit(0);
    ff1.invert = data.bit(1);
    if(data.bit(2,3) == 0) ff1 = !ff1;
    if(data.bit(2,3) == 1) ff1 = 1;
    if(data.bit(2,3) == 2) ff1 = 0;
    ff3.source = data.bit(4);
    ff3.invert = data.bit(5);
    if(data.bit(6,7) == 0) ff3 = !ff3;
    if(data.bit(6,7) == 1) ff3 = 1;
    if(data.bit(6,7) == 2) ff3 = 0;
    return;

  //TREG2
  case 0x26:
    if(!t23.buffer.enable) t2.compare = data;
    t23.buffer.compare = data;
    return;

  //TREG3
  case 0x27:
    t3.compare = data;
    return;

  //T23MOD
  case 0x28:
    t2.mode  = data.bit(0,1);
    t3.mode  = data.bit(2,3);
    t23.pwm  = data.bit(4,5);
    t23.mode = data.bit(6,7);
    return;

  //TRDC
  case 0x29:
    t01.buffer.enable = data.bit(0);
    t23.buffer.enable = data.bit(1);
    return;

  //PACR
  case 0x2c:
    pa0.flow = data.bit(0);
    pa1.flow = data.bit(1);
    pa2.flow = data.bit(2);
    pa3.flow = data.bit(3);
    return;

  //PAFC
  case 0x2d:
    pa2.mode = data.bit(2);
    pa3.mode = data.bit(3);
    return;

  //PBCR
  case 0x2e:
    pb0.flow = data.bit(0);
    pb1.flow = data.bit(1);
    pb2.flow = data.bit(2);
    pb3.flow = data.bit(3);
    pb4.flow = data.bit(4);
    pb5.flow = data.bit(5);
    pb6.flow = data.bit(6);
    pb7.flow = data.bit(7);
    return;

  //PBFC
  case 0x2f:
    pb2.mode = data.bit(2);
    pb3.mode = data.bit(3);
    pb6.mode = data.bit(6);
    return;

  //TREG4
  case 0x30:
    if(!t4.buffer.enable) t4.compare4.byte(0) = data;
    t4.buffer.compare.byte(0) = data;
    return;
  case 0x31:
    if(!t4.buffer.enable) t4.compare4.byte(1) = data;
    t4.buffer.compare.byte(1) = data;
    return;

  //TREG5
  case 0x32: t4.compare5.byte(0) = data; return;
  case 0x33: t4.compare5.byte(1) = data; return;

  //CAP1
  case 0x34: t4.capture1.byte(0) = data; return;
  case 0x35: t4.capture1.byte(1) = data; return;

  //CAP2
  case 0x36: t4.capture2.byte(0) = data; return;
  case 0x37: t4.capture2.byte(1) = data; return;

  //T4MOD
  case 0x38:
    t4.mode = data.bit(0,1);
    t4.clearOnCompare5 = data.bit(2);
    t4.captureMode = data.bit(3,4);
    if(!data.bit(5)) t4.captureTo1();
    ff5.flipOnCompare5 = data.bit(6);
    ff5.flipOnCapture2 = data.bit(7);
    int4.edge.rising  = t4.captureMode != 2;
    int4.edge.falling = t4.captureMode == 2;
    return;

  //T4FFCR
  case 0x39:
    if(data.bit(0,1) == 0) ff4 = !ff4;
    if(data.bit(0,1) == 1) ff4 = 1;
    if(data.bit(0,1) == 2) ff4 = 0;
    ff4.flipOnCompare4 = data.bit(2);
    ff4.flipOnCompare5 = data.bit(3);
    ff4.flipOnCapture1 = data.bit(4);
    ff4.flipOnCapture2 = data.bit(5);
    if(data.bit(6,7) == 0) ff5 = !ff5;
    if(data.bit(6,7) == 1) ff5 = 1;
    if(data.bit(6,7) == 2) ff5 = 0;
    return;

  //T45CR
  case 0x3a:
    t4.buffer.enable = data.bit(0);
    t5.buffer.enable = data.bit(1);
    return;

  //MSAR0
  case 0x3c:
    cs0.address.bit(16,23) = data;
    return;

  //MAMR0
  case 0x3d:
    cs0.mask.bit( 8) = data.bit(0);
    cs0.mask.bit( 9) = data.bit(1);  //9..
    cs0.mask.bit(10) = data.bit(1);
    cs0.mask.bit(11) = data.bit(1);
    cs0.mask.bit(12) = data.bit(1);
    cs0.mask.bit(13) = data.bit(1);
    cs0.mask.bit(14) = data.bit(1);  //..14
    cs0.mask.bit(15) = data.bit(2);
    cs0.mask.bit(16) = data.bit(3);
    cs0.mask.bit(17) = data.bit(4);
    cs0.mask.bit(18) = data.bit(5);
    cs0.mask.bit(19) = data.bit(6);
    cs0.mask.bit(20) = data.bit(7);
    return;

  //MSAR1
  case 0x3e:
    cs1.address.bit(16,23) = data;
    return;

  //MAMR1
  case 0x3f:
    cs1.mask.bit( 8) = data.bit(0);
    cs1.mask.bit( 9) = data.bit(1);  //9..
    cs1.mask.bit(10) = data.bit(1);
    cs1.mask.bit(11) = data.bit(1);
    cs1.mask.bit(12) = data.bit(1);
    cs1.mask.bit(13) = data.bit(1);
    cs1.mask.bit(14) = data.bit(1);
    cs1.mask.bit(15) = data.bit(1);  //..15
    cs1.mask.bit(16) = data.bit(2);
    cs1.mask.bit(17) = data.bit(3);
    cs1.mask.bit(18) = data.bit(4);
    cs1.mask.bit(19) = data.bit(5);
    cs1.mask.bit(20) = data.bit(6);
    cs1.mask.bit(21) = data.bit(7);
    return;

  //TREG6
  case 0x40:
    if(!t5.buffer.enable) t5.compare6.byte(0) = data;
    t5.buffer.compare.byte(0) = data;
    return;
  case 0x41:
    if(!t5.buffer.enable) t5.compare6.byte(1) = data;
    t5.buffer.compare.byte(1) = data;
    return;

  //TREG7
  case 0x42: t5.compare7.byte(0) = data; return;
  case 0x43: t5.compare7.byte(1) = data; return;

  //CAP3
  case 0x44: t5.capture3.byte(0) = data; return;
  case 0x45: t5.capture3.byte(1) = data; return;

  //CAP4
  case 0x46: t5.capture4.byte(0) = data; return;
  case 0x47: t5.capture4.byte(1) = data; return;

  //T5MOD
  case 0x48:
    t5.mode = data.bit(0,1);
    t5.clearOnCompare7 = data.bit(2);
    t5.captureMode = data.bit(3,4);
    if(!data.bit(5)) t5.captureTo3();
    int6.edge.rising  = t5.captureMode != 2;
    int6.edge.falling = t5.captureMode == 2;
    return;

  //T5FFCR
  case 0x49:
    if(data.bit(0,1) == 0) ff6 = !ff6;
    if(data.bit(0,1) == 1) ff6 = 1;
    if(data.bit(0,1) == 2) ff6 = 0;
    ff6.flipOnCompare6 = data.bit(2);
    ff6.flipOnCompare7 = data.bit(3);
    ff6.flipOnCapture3 = data.bit(4);
    ff6.flipOnCapture4 = data.bit(5);
    return;

  //MSAR2
  case 0x5c:
    cs2.address.bit(16,23) = data;
    return;

  //MAMR2
  case 0x5d:
    cs2.mask.bit(15,22) = data;
    return;

  //MSAR3
  case 0x5e:
    cs3.address.bit(16,23) = data;
    return;

  //MAMR3
  case 0x5f:
    cs3.mask.bit(15,22) = data;
    return;

  //B0CS
  case 0x68:
    cs0.timing = data.bit(0,1);
    cs0.width = data.bit(2) ? Byte : Word;
    cs0.enable = data.bit(4);
    return;

  //B1CS
  case 0x69:
    cs1.timing = data.bit(0,1);
    cs1.width = data.bit(2) ? Byte : Word;
    cs1.enable = data.bit(4);
    return;

  //B2CS
  case 0x6a:
    cs2.timing = data.bit(0,1);
    cs2.width = data.bit(2) ? Byte : Word;
    cs2.mode = data.bit(3);
    cs2.enable = data.bit(4);
    return;

  //B3CS
  case 0x6b:
    cs3.timing = data.bit(0,1);
    cs3.width = data.bit(2) ? Byte : Word;
    cs3.cas = data.bit(3);
    cs3.enable = data.bit(4);
    return;

  //BEXCS
  case 0x6c:
    csx.timing = data.bit(0,1);
    csx.width = data.bit(2) ? Byte : Word;
    return;

  //ADMOD
  case 0x6d: {
    uint1 busy = adc.busy;
    adc.channel = data.bit(0,1);
    uint1 start = data.bit(2);
    adc.speed = data.bit(3);
    adc.scan = data.bit(4);
    adc.repeat = data.bit(5);

    if(!busy && start) {
      adc.busy = 1;
      adc.counter = 0;
    }
  } return;

  //WDMOD
  case 0x6e:
    watchdog.drive = data.bit(0);
    watchdog.reset = data.bit(1);
    watchdog.standby = data.bit(2,3);
    watchdog.warmup = data.bit(4);
    watchdog.frequency = data.bit(5,6);
    watchdog.enable = data.bit(7);
    if(watchdog.enable) watchdog.counter = 0;  //todo: is this only on 0->1 transitions?
    return;

  //WDCR
  case 0x6f:
    if(data == 0x4e) watchdog.counter = 0;
    if(data == 0xb1) watchdog.enable = 0;
    return;

  //INTE0AD
  case 0x70:
    int0.setPriority(data.bit(0,2));
    if(!data.bit(3)) int0.clear();
    intad.setPriority(data.bit(4,6));
    if(!data.bit(7)) intad.clear();
    return;

  //INTE45
  case 0x71:
    int4.setPriority(data.bit(0,2));
    if(!data.bit(3)) int4.clear();
    int5.setPriority(data.bit(4,6));
    if(!data.bit(7)) int5.clear();
    return;

  //INTE67
  case 0x72:
    int6.setPriority(data.bit(0,2));
    if(!data.bit(3)) int6.clear();
    int7.setPriority(data.bit(4,6));
    if(!data.bit(7)) int7.clear();
    return;

  //INTET01
  case 0x73:
    intt0.setPriority(data.bit(0,2));
    if(!data.bit(3)) intt0.clear();
    intt1.setPriority(data.bit(4,6));
    if(!data.bit(7)) intt1.clear();
    return;

  //INTET23
  case 0x74:
    intt2.setPriority(data.bit(0,2));
    if(!data.bit(3)) intt2.clear();
    intt3.setPriority(data.bit(4,6));
    if(!data.bit(7)) intt3.clear();
    return;

  //INTET45
  case 0x75:
    inttr4.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttr4.clear();
    inttr5.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttr5.clear();
    return;

  //INTET67
  case 0x76:
    inttr6.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttr6.clear();
    inttr7.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttr7.clear();
    return;

  //INTE50
  case 0x77:
    intrx0.setPriority(data.bit(0,2));
    if(!data.bit(3)) intrx0.clear();
    inttx0.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttx0.clear();
    return;

  //INTE51
  case 0x78:
    intrx1.setPriority(data.bit(0,2));
    if(!data.bit(3)) intrx1.clear();
    inttx1.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttx1.clear();
    return;

  //INTETC01
  case 0x79:
    inttc0.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttc0.clear();
    inttc1.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttc1.clear();
    return;

  //INTETC23
  case 0x7a:
    inttc2.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttc2.clear();
    inttc3.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttc3.clear();
    return;

  //IIMC
  case 0x7b:
    nmi.edge.rising = data.bit(0);
    int0.edge.rising = !data.bit(1);
    int0.level.high = data.bit(1);
    int0.setEnable(data.bit(2));
    interrupts.poll();
    return;

  //DMA0V
  case 0x7c:
    dma0.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    return;

  //DMA1V
  case 0x7d:
    dma1.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    return;

  //DMA2V
  case 0x7e:
    dma2.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    return;

  //DMA3V
  case 0x7f:
    dma3.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    return;

  case 0x80:
    clock.rate = data.bit(0,2);
    return;

  case 0x90: rtc.enable  = data.bit(0); return;
  case 0x91: rtc.year    = data; return;
  case 0x92: rtc.month   = data; return;
  case 0x93: rtc.day     = data; return;
  case 0x94: rtc.hour    = data; return;
  case 0x95: rtc.minute  = data; return;
  case 0x96: rtc.second  = data; return;
  case 0x97: rtc.weekday = data.bit(0,3); return;

  case 0xa0:
    psg.writeRight(data);
    return;

  case 0xa1:
    psg.writeLeft(data);
    return;

  case 0xa2:
    psg.writeRightDAC(data);
    return;

  case 0xa3:
    psg.writeLeftDAC(data);
    return;

  case 0xb2:
    misc.rtsDisable = data.bit(0);
    return;

  case 0xb3:
    nmi.setEnable(data.bit(2));
    return;

  case 0xb4:
    misc.b4 = data;
    return;

  case 0xb5:
    misc.b5 = data;
    return;

  case 0xb8:
    if(data == 0x55) psg.enablePSG();
    if(data == 0xaa) psg.enableDAC();
    return;

  case 0xb9:
    if(data == 0x55) apu.enable();
    if(data == 0xaa) apu.disable();
    return;

  case 0xba:
    apu.nmi.line = 1;
    return;

  case 0xbc:
    apu.port.data = data;
    return;
  }

//print("CPU::writeIO(", hex(address, 2L), " = ", hex(data, 2L), ")\n");
}
