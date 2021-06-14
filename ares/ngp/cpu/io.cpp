auto CPU::readIO(n8 address) -> n8 {
  n8 data;

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
    break;

  //P1CR (write-only)
  case 0x04: break;

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
    break;

  //P2FC (write-only)
  case 0x09: break;

  //P5
  case 0x0d:
    data.bit(0) = misc.p5;
    data.bit(2) = p52;
    data.bit(3) = p53;
    data.bit(4) = p54;
    data.bit(5) = p55;
    break;

  //P5CR (write-only)
  case 0x10: break;

  //P5SC (write-only)
  case 0x11: break;

  //P6
  case 0x12:
    data.bit(0) = p60;
    data.bit(1) = p61;
    data.bit(2) = p62;
    data.bit(3) = p63;
    data.bit(4) = p64;
    data.bit(5) = p65;
    break;

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
    break;

  //P6FC (write-only)
  case 0x15: break;

  //P7FC (write-only)
  case 0x17: break;

  //P8
  case 0x18:
    data.bit(0) = p80;
    data.bit(1) = p81;
    data.bit(2) = p82;
    data.bit(3) = p83;
    data.bit(4) = p84;
    data.bit(5) = p85;
    break;

  //P9
  case 0x19:
    data.bit(0) = p90;
    data.bit(1) = p91;
    data.bit(2) = p92;
    data.bit(3) = p93;
    break;

  //P8CR (write-only)
  case 0x1a: break;

  //P8FC (write-only)
  case 0x1b: break;

  //PA
  case 0x1e:
    data.bit(0) = pa0;
    data.bit(1) = pa1;
    data.bit(2) = pa2;
    data.bit(3) = pa3;
    break;

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
    break;

  //TRUN
  case 0x20:
    data.bit(0) = t0.enable;
    data.bit(1) = t1.enable;
    data.bit(2) = t2.enable;
    data.bit(3) = t3.enable;
    data.bit(4) = t4.enable;
    data.bit(5) = t5.enable;
    data.bit(7) = prescaler.enable;
    break;

  //TREG0 (write-only)
  case 0x22: break;

  //TREG1 (write-only)
  case 0x23: break;

  //T01MOD
  case 0x24:
    data.bit(0,1) = t0.mode;
    data.bit(2,3) = t1.mode;
    data.bit(4,5) = t01.pwm;
    data.bit(6,7) = t01.mode;
    break;

  //TFFCR
  case 0x25:
    data.bit(0)   = ff1.source;
    data.bit(1)   = ff1.invert;
    data.bit(2,3) = 0b11;  //write-only
    data.bit(4)   = ff3.source;
    data.bit(5)   = ff3.invert;
    data.bit(6,7) = 0b11;  //write-only
    break;

  //TREG2 (write-only)
  case 0x26: break;

  //TREG3 (write-only)
  case 0x27: break;

  //T23MOD
  case 0x28:
    data.bit(0,1) = t2.mode;
    data.bit(2,3) = t3.mode;
    data.bit(4,5) = t23.pwm;
    data.bit(6,7) = t23.mode;
    break;

  //TRDC
  case 0x29:
    data.bit(0) = t01.buffer.enable;
    data.bit(1) = t23.buffer.enable;
    break;

  //PACR (write-only)
  case 0x2c: break;

  //PAFC (write-only)
  case 0x2d: break;

  //PBCR (write-only)
  case 0x2e: break;

  //PBFC (write-only)
  case 0x2f: break;

  //TREG4 (write-only?)
  case 0x30: break;
  case 0x31: break;

  //TREG5 (write-only?)
  case 0x32: break;
  case 0x33: break;

  //CAP1
  case 0x34: data = t4.capture1.byte(0); break;
  case 0x35: data = t4.capture1.byte(1); break;

  //CAP2
  case 0x36: data = t4.capture2.byte(0); break;
  case 0x37: data = t4.capture2.byte(1); break;

  //T4MOD
  case 0x38:
    data.bit(0,1) = t4.mode;
    data.bit(2)   = t4.clearOnCompare5;
    data.bit(3,4) = t4.captureMode;
    data.bit(5)   = 1;
    data.bit(6)   = ff5.flipOnCompare5;
    data.bit(7)   = ff5.flipOnCapture2;
    break;

  //T4FFCR
  case 0x39:
    data.bit(0,1) = 0b11;
    data.bit(2)   = ff4.flipOnCompare4;
    data.bit(3)   = ff4.flipOnCompare5;
    data.bit(4)   = ff4.flipOnCapture1;
    data.bit(5)   = ff4.flipOnCapture2;
    data.bit(6,7) = 0b11;
    break;

  //T45CR
  case 0x3a:
    data.bit(0) = t4.buffer.enable;
    data.bit(1) = t5.buffer.enable;
    data.bit(2) = pg0.shiftTrigger;
    data.bit(3) = pg1.shiftTrigger;
    break;

  //MSAR0
  case 0x3c:
    data.bit(0,7) = cs0.address.bit(16,23);
    break;

  //MAMR0
  case 0x3d:
    data.bit(0,1) = cs0.mask.bit(8,9);
    data.bit(2,7) = cs0.mask.bit(15,20);
    break;

  //MSAR1
  case 0x3e:
    data.bit(0,7) = cs1.address.bit(16,23);
    break;

  //MAMR1
  case 0x3f:
    data.bit(0,1) = cs1.mask.bit(8,9);
    data.bit(2,7) = cs1.mask.bit(16,21);
    break;

  //TREG6 (write-only?)
  case 0x40: break;
  case 0x41: break;

  //TREG7 (write-only?)
  case 0x42: break;
  case 0x43: break;

  //CAP3
  case 0x44: data = t5.capture3.byte(0); break;
  case 0x45: data = t5.capture4.byte(1); break;

  //CAP4
  case 0x46: data = t5.capture3.byte(0); break;
  case 0x47: data = t5.capture4.byte(1); break;

  //T5MOD
  case 0x48:
    data.bit(0,1) = t5.mode;
    data.bit(2)   = t5.clearOnCompare7;
    data.bit(3,4) = t5.captureMode;
    data.bit(5)   = 1;
    break;

  //T5FFCR
  case 0x49:
    data.bit(0,1) = 0b11;
    data.bit(2)   = ff6.flipOnCompare6;
    data.bit(3)   = ff6.flipOnCompare7;
    data.bit(4)   = ff6.flipOnCapture3;
    data.bit(5)   = ff6.flipOnCapture4;
    data.bit(6,7) = 0b11;
    break;

  //PG0REG
  case 0x4c:
    data.bit(0,3) = pg0.shiftAlternateRegister;
    break;

  //PG1REG
  case 0x4d:
    data.bit(0,3) = pg1.shiftAlternateRegister;
    break;

  //PG01CR
  case 0x4e:
    data.bit(0) = pg0.triggerInputEnable;
    data.bit(1) = pg0.excitationMode;
    data.bit(2) = pg0.rotatingDirection;
    data.bit(3) = pg0.writeMode;
    data.bit(4) = pg1.triggerInputEnable;
    data.bit(5) = pg1.excitationMode;
    data.bit(6) = pg1.rotatingDirection;
    data.bit(7) = pg1.writeMode;
    break;

  //SC0BUF
  case 0x50:
    data = sc0.receive();
    break;

  //SC0CR
  case 0x51:
    data.bit(0) = sc0.inputClock;
    data.bit(1) = sc0.clockEdge;
    data.bit(2) = sc0.framingError;
    data.bit(3) = sc0.parityError;
    data.bit(4) = sc0.overrunError;
    data.bit(5) = sc0.parityAddition;
    data.bit(6) = sc0.parity;
    data.bit(7) = sc0.receiveBit8;
    sc0.framingError = 0;
    sc0.parityError  = 0;
    sc0.overrunError = 0;
    break;

  //SC0MOD
  case 0x52:
    data.bit(0,1) = sc0.clock;
    data.bit(2,3) = sc0.mode;
    data.bit(4)   = sc0.wakeUp;
    data.bit(5)   = sc0.receiving;
    data.bit(6)   = sc0.handshake;
    data.bit(7)   = sc0.transferBit8;
    break;

  //BR0CR
  case 0x53:
    data.bit(0,3) = sc0.baudRateDividend;
    data.bit(4,5) = sc0.baudRateDivider;
    break;

  //SC1BUF
  case 0x54:
    data = sc1.receive();
    break;

  //SC1CR
  case 0x55:
    data.bit(0) = sc1.inputClock;
    data.bit(1) = sc1.clockEdge;
    data.bit(2) = sc1.framingError;
    data.bit(3) = sc1.parityError;
    data.bit(4) = sc1.overrunError;
    data.bit(5) = sc1.parityAddition;
    data.bit(6) = sc1.parity;
    data.bit(7) = sc1.receiveBit8;
    sc1.framingError = 0;
    sc1.parityError  = 0;
    sc1.overrunError = 0;
    break;

  //SC1MOD
  case 0x56:
    data.bit(0,1) = sc1.clock;
    data.bit(2,3) = sc1.mode;
    data.bit(4)   = sc1.wakeUp;
    data.bit(5)   = sc1.receiving;
    data.bit(6)   = sc1.handshake;  //always 0
    data.bit(7)   = sc1.transferBit8;
    break;

  //BR1CR
  case 0x57:
    data.bit(0,3) = sc1.baudRateDividend;
    data.bit(4,5) = sc1.baudRateDivider;
    break;

  //ODE
  case 0x58:
    data.bit(0) = p80.drain;
    data.bit(1) = p83.drain;
    break;

  //DREFCR
  case 0x5a:
    data.bit(0)   = dram.refreshCycle;
    data.bit(1,3) = dram.refreshCycleWidth;
    data.bit(4,6) = dram.refreshCycleInsertion;
    data.bit(7)   = dram.dummyCycle;
    break;

  //DMEMCR
  case 0x5b:
    data.bit(0)   = dram.memoryAccessEnable;
    data.bit(1,2) = dram.multiplexAddressLength;
    data.bit(3)   = dram.multiplexAddressEnable;
    data.bit(4)   = dram.memoryAccessSpeed;
    data.bit(5)   = dram.busReleaseMode;
    break;

  //MSAR2
  case 0x5c:
    data.bit(0,7) = cs2.address.bit(16,23);
    break;

  //MAMR2
  case 0x5d:
    data.bit(0,7) = cs2.mask.bit(15,22);
    break;

  //MSAR3
  case 0x5e:
    data.bit(0,7) = cs3.address.bit(16,23);
    break;

  //MAMR3
  case 0x5f:
    data.bit(0,7) = cs3.mask.bit(15,22);
    break;

  //ADREG04L
  case 0x60:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[0].bit(0,1);
    adc.end = 0;
    break;

  //ADREG04H
  case 0x61:
    data.bit(0,7) = adc.result[0].bit(2,9);
    adc.end = 0;
    intad.clear();
    break;

  //ADREG15L
  case 0x62:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[1].bit(0,1);
    adc.end = 0;
    break;

  //ADREG15H
  case 0x63:
    data.bit(0,7) = adc.result[1].bit(2,9);
    adc.end = 0;
    intad.clear();
    break;

  //ADREG2L
  case 0x64:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[2].bit(0,1);
    adc.end = 0;
    break;

  //ADREG2H
  case 0x65:
    data.bit(0,7) = adc.result[2].bit(2,9);
    adc.end = 0;
    intad.clear();
    break;

  //ADREG3L
  case 0x66:
    data.bit(0,5) = 0b111111;
    data.bit(6,7) = adc.result[3].bit(0,1);
    adc.end = 0;
    break;

  //ADREG3H
  case 0x67:
    data.bit(0,7) = adc.result[3].bit(2,9);
    adc.end = 0;
    intad.clear();
    break;

  //B0CS (write-only)
  case 0x68: break;

  //B1CS (write-only)
  case 0x69: break;

  //B2CS (write-only)
  case 0x6a: break;

  //B3CS (write-only)
  case 0x6b: break;

  //BEXCS (write-only)
  case 0x6c: break;

  //ADMOD
  case 0x6d:
    data.bit(0,1) = adc.channel;
    data.bit(2)   = 0;  //always reads as zero
    data.bit(3)   = adc.speed;
    data.bit(4)   = adc.scan;
    data.bit(5)   = adc.repeat;
    data.bit(6)   = adc.busy;
    data.bit(7)   = adc.end;
    break;

  //WDMOD
  case 0x6e:
    data.bit(0)   = watchdog.drive;
    data.bit(1)   = watchdog.reset;
    data.bit(2,3) = watchdog.standby;
    data.bit(4)   = watchdog.warmup;
    data.bit(5,6) = watchdog.frequency;
    data.bit(7)   = watchdog.enable;
    break;

  //WDCR (write-only)
  case 0x6f: break;

  //INTE0AD
  case 0x70:
    data.bit(0,2) = 0b000;
    data.bit(3)   = int0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = intad.pending;
    break;

  //INTE45
  case 0x71:
    data.bit(0,2) = 0b000;
    data.bit(3)   = int4.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = int5.pending;
    break;

  //INTE67
  case 0x72:
    data.bit(0,2) = 0b000;
    data.bit(3)   = int6.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = int7.pending;
    break;

  //INTET01
  case 0x73:
    data.bit(0,2) = 0b000;
    data.bit(3)   = intt0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = intt1.pending;
    break;

  //INTET23
  case 0x74:
    data.bit(0,2) = 0b000;
    data.bit(3)   = intt2.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = intt3.pending;
    break;

  //INTET45
  case 0x75:
    data.bit(0,2) = 0b000;
    data.bit(3)   = inttr4.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttr5.pending;
    break;

  //INTET67
  case 0x76:
    data.bit(0,2) = 0b000;
    data.bit(3)   = inttr6.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttr7.pending;
    break;

  //INTE50
  case 0x77:
    data.bit(0,2) = 0b000;
    data.bit(3)   = intrx0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttx0.pending;
    break;

  //INTE51
  case 0x78:
    data.bit(0,2) = 0b000;
    data.bit(3)   = intrx1.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttx1.pending;
    break;

  //INTETC01
  case 0x79:
    data.bit(0,2) = 0b000;
    data.bit(3)   = inttc0.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttc1.pending;
    break;

  //INTETC23
  case 0x7a:
    data.bit(0,2) = 0b000;
    data.bit(3)   = inttc2.pending;
    data.bit(4,6) = 0b000;
    data.bit(7)   = inttc3.pending;
    break;

  //IIMC
  case 0x7b:
    data.bit(0) = nmi.edge.rising;
    data.bit(1) = int0.level.high;
    data.bit(2) = int0.enable;
    break;

  //DMA0V (write-only)
  case 0x7c: break;

  //DMA1V (write-only)
  case 0x7d: break;

  //DMA2V (write-only)
  case 0x7e: break;

  //DMA3V (write-only)
  case 0x7f: break;

  case 0x80:
    data.bit(0,2) = clock.rate;  //unconfirmed
    data.bit(3,7) = 0b00000;     //unconfirmed
    break;

  case 0x90: data = rtc.enable; break;
  case 0x91: data = rtc.year;   break;
  case 0x92: data = rtc.month;  break;
  case 0x93: data = rtc.day;    break;
  case 0x94: data = rtc.hour;   break;
  case 0x95: data = rtc.minute; break;
  case 0x96: data = rtc.second; break;
  case 0x97:
    data.bit(0,3) = rtc.weekday;
    data.bit(4,7) = rtc.year & 3;
    break;

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
    break;

  case 0xb1:
    //power button polled in CPU::pollPowerButton()
    data.bit(0) = !system.controls.power->value();
    data.bit(1) = 1;  //sub battery (CR2032)
    data.bit(2) = 1;  //unknown (must be 1 or SNK Gals' Fighter displays a link connection error)
    break;

  case 0xb2:
    data.bit(0) = misc.rtsDisable;
    break;

  case 0xb3:
    data.bit(2) = nmi.enable;
    break;

  case 0xb4: data = unknown.b4; break;
  case 0xb5: data = unknown.b5; break;
  case 0xb6: data = unknown.b6; break;
  case 0xb7: data = unknown.b7; break;

  case 0xbc:
    data = apu.port.data;
    break;

  }

  debugger.readIO(address, data);
  return data;
}

auto CPU::writeIO(n8 address, n8 data) -> void {
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
    break;

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
    break;

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
    break;

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
    break;

  //P5
  case 0x0d:
    misc.p5 = data.bit(0);
    p52     = data.bit(2);
    p53     = data.bit(3);
    p54     = data.bit(4);
    p55     = data.bit(5);
    break;

  //P5CR
  case 0x10:
    p52.flow = data.bit(2);
    p53.flow = data.bit(3);
    p54.flow = data.bit(4);
    p55.flow = data.bit(5);
    break;

  //P5FC
  case 0x11:
    p52.mode = data.bit(2);
    p53.mode = data.bit(3);
    p54.mode = data.bit(4);
    p55.mode = data.bit(5);
    break;

  //P6
  case 0x12:
    p60 = data.bit(0);
    p61 = data.bit(1);
    p62 = data.bit(2);
    p63 = data.bit(3);
    p64 = data.bit(4);
    p65 = data.bit(5);
    break;

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
    break;

  //P6FC
  case 0x15:
    p60.mode = data.bit(0);
    p61.mode = data.bit(1);
    p62.mode = data.bit(2);
    p63.mode = data.bit(3);
    p64.mode = data.bit(4);
    p65.mode = data.bit(5);
    break;

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
    break;

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
    break;

  //P8
  case 0x18:
    p80 = data.bit(0);
    p81 = data.bit(1);
    p82 = data.bit(2);
    p83 = data.bit(3);
    p84 = data.bit(4);
    p85 = data.bit(5);
    break;

  //P9 (read-only)
  case 0x19: break;

  //P8CR
  case 0x1a:
    p80.flow = data.bit(0);
    p81.flow = data.bit(1);
    p82.flow = data.bit(2);
    p83.flow = data.bit(3);
    p84.flow = data.bit(4);
    p85.flow = data.bit(5);
    break;

  //P8FC
  case 0x1b:
    p80.mode = data.bit(0);
    p82.mode = data.bit(2);
    p83.mode = data.bit(3);
    p85.mode = data.bit(5);
    break;

  //PA
  case 0x1e:
    pa0 = data.bit(0);
    pa1 = data.bit(1);
    if(pa2.mode == 0) pa2 = data.bit(2);
    if(pa2.mode == 0) pa3 = data.bit(3);
    break;

  //PB
  case 0x1f:
    pb0 = data.bit(0);
    pb1 = data.bit(1);
    break;

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
    break;

  //TREG0
  case 0x22:
    if(!t01.buffer.enable) t0.compare = data;
    t01.buffer.compare = data;
    break;

  //TREG1
  case 0x23:
    t1.compare = data;
    break;

  //T01MOD
  case 0x24:
    t0.mode  = data.bit(0,1);
    t1.mode  = data.bit(2,3);
    t01.pwm  = data.bit(4,5);
    t01.mode = data.bit(6,7);
    break;

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
    break;

  //TREG2
  case 0x26:
    if(!t23.buffer.enable) t2.compare = data;
    t23.buffer.compare = data;
    break;

  //TREG3
  case 0x27:
    t3.compare = data;
    break;

  //T23MOD
  case 0x28:
    t2.mode  = data.bit(0,1);
    t3.mode  = data.bit(2,3);
    t23.pwm  = data.bit(4,5);
    t23.mode = data.bit(6,7);
    break;

  //TRDC
  case 0x29:
    t01.buffer.enable = data.bit(0);
    t23.buffer.enable = data.bit(1);
    break;

  //PACR
  case 0x2c:
    pa0.flow = data.bit(0);
    pa1.flow = data.bit(1);
    pa2.flow = data.bit(2);
    pa3.flow = data.bit(3);
    break;

  //PAFC
  case 0x2d:
    pa2.mode = data.bit(2);
    pa3.mode = data.bit(3);
    break;

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
    break;

  //PBFC
  case 0x2f:
    pb2.mode = data.bit(2);
    pb3.mode = data.bit(3);
    pb6.mode = data.bit(6);
    break;

  //TREG4
  case 0x30:
    if(!t4.buffer.enable) t4.compare4.byte(0) = data;
    t4.buffer.compare.byte(0) = data;
    break;
  case 0x31:
    if(!t4.buffer.enable) t4.compare4.byte(1) = data;
    t4.buffer.compare.byte(1) = data;
    break;

  //TREG5
  case 0x32: t4.compare5.byte(0) = data; break;
  case 0x33: t4.compare5.byte(1) = data; break;

  //CAP1
  case 0x34: t4.capture1.byte(0) = data; break;
  case 0x35: t4.capture1.byte(1) = data; break;

  //CAP2
  case 0x36: t4.capture2.byte(0) = data; break;
  case 0x37: t4.capture2.byte(1) = data; break;

  //T4MOD
  case 0x38:
    t4.mode            = data.bit(0,1);
    t4.clearOnCompare5 = data.bit(2);
    t4.captureMode     = data.bit(3,4);
    if(!data.bit(5)) t4.captureTo1();
    ff5.flipOnCompare5 = data.bit(6);
    ff5.flipOnCapture2 = data.bit(7);
    int4.edge.rising  = t4.captureMode != 2;
    int4.edge.falling = t4.captureMode == 2;
    break;

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
    break;

  //T45CR
  case 0x3a:
    t4.buffer.enable = data.bit(0);
    t5.buffer.enable = data.bit(1);
    pg0.shiftTrigger = data.bit(2);
    pg1.shiftTrigger = data.bit(3);
    break;

  //MSAR0
  case 0x3c:
    cs0.address.bit(16,23) = data;
    break;

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
    break;

  //MSAR1
  case 0x3e:
    cs1.address.bit(16,23) = data;
    break;

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
    break;

  //TREG6
  case 0x40:
    if(!t5.buffer.enable) t5.compare6.byte(0) = data;
    t5.buffer.compare.byte(0) = data;
    break;
  case 0x41:
    if(!t5.buffer.enable) t5.compare6.byte(1) = data;
    t5.buffer.compare.byte(1) = data;
    break;

  //TREG7
  case 0x42: t5.compare7.byte(0) = data; break;
  case 0x43: t5.compare7.byte(1) = data; break;

  //CAP3
  case 0x44: t5.capture3.byte(0) = data; break;
  case 0x45: t5.capture3.byte(1) = data; break;

  //CAP4
  case 0x46: t5.capture4.byte(0) = data; break;
  case 0x47: t5.capture4.byte(1) = data; break;

  //T5MOD
  case 0x48:
    t5.mode            = data.bit(0,1);
    t5.clearOnCompare7 = data.bit(2);
    t5.captureMode     = data.bit(3,4);
    if(!data.bit(5)) t5.captureTo3();
    int6.edge.rising  = t5.captureMode != 2;
    int6.edge.falling = t5.captureMode == 2;
    break;

  //T5FFCR
  case 0x49:
    if(data.bit(0,1) == 0) ff6 = !ff6;
    if(data.bit(0,1) == 1) ff6 = 1;
    if(data.bit(0,1) == 2) ff6 = 0;
    ff6.flipOnCompare6 = data.bit(2);
    ff6.flipOnCompare7 = data.bit(3);
    ff6.flipOnCapture3 = data.bit(4);
    ff6.flipOnCapture4 = data.bit(5);
    break;

  //PG0REG
  case 0x4c:
    pg0.shiftAlternateRegister  = data.bit(0,3);
    pg0.patternGenerationOutput = data.bit(4,7);
    break;

  //PG1REG
  case 0x4d:
    pg1.shiftAlternateRegister  = data.bit(0,3);
    pg1.patternGenerationOutput = data.bit(4,7);
    break;

  //PG01CR
  case 0x4e:
    pg0.triggerInputEnable = data.bit(0);
    pg0.excitationMode     = data.bit(1);
    pg0.rotatingDirection  = data.bit(2);
    pg0.writeMode          = data.bit(3);
    pg1.triggerInputEnable = data.bit(4);
    pg1.excitationMode     = data.bit(5);
    pg1.rotatingDirection  = data.bit(6);
    pg1.writeMode          = data.bit(7);
    break;

  //SC0BUF
  case 0x50:
    sc0.transmit(data);
    break;

  //SC0CR
  case 0x51:
    sc0.inputClock     = data.bit(0);
    sc0.clockEdge      = data.bit(1);
    sc0.parityAddition = data.bit(5);
    sc0.parity         = data.bit(6);
    break;

  //SC0MOD
  case 0x52:
    sc0.clock        = data.bit(0,1);
    sc0.mode         = data.bit(2,3);
    sc0.wakeUp       = data.bit(4);
    sc0.receiving    = data.bit(5);
    sc0.handshake    = data.bit(6);
    sc0.transferBit8 = data.bit(7);
    break;

  //BR0CR
  case 0x53:
    sc0.baudRateDividend = data.bit(0,3);
    sc0.baudRateDivider  = data.bit(4,5);
    break;

  //SC1BUF
  case 0x54:
    sc1.transmit(data);
    break;

  //SC1CR
  case 0x55:
    sc1.inputClock     = data.bit(0);
    sc1.clockEdge      = data.bit(1);
    sc1.parityAddition = data.bit(5);
    sc1.parity         = data.bit(6);
    break;

  //SC1MOD
  case 0x56:
    sc1.clock        = data.bit(0,1);
    sc1.mode         = data.bit(2,3);
    sc1.wakeUp       = data.bit(4);
    sc1.receiving    = data.bit(5);
  //sc1.handshake    = 0;  //fixed at 0
    sc1.transferBit8 = data.bit(7);
    break;

  //BR1CR
  case 0x57:
    sc1.baudRateDividend = data.bit(0,3);
    sc1.baudRateDivider  = data.bit(4,5);
    break;

  //ODE
  case 0x58:
    p80.drain = data.bit(0);
    p83.drain = data.bit(1);
    break;

  //DREFCR
  case 0x5a:
    dram.refreshCycle          = data.bit(0);
    dram.refreshCycleWidth     = data.bit(1,3);
    dram.refreshCycleInsertion = data.bit(4,6);
    dram.dummyCycle            = data.bit(7);
    break;

  //DMEMCR
  case 0x5b:
    dram.memoryAccessEnable     = data.bit(0);
    dram.multiplexAddressLength = data.bit(1,2);
    dram.multiplexAddressEnable = data.bit(3);
    dram.memoryAccessSpeed      = data.bit(4);
    dram.busReleaseMode         = data.bit(5);
    dram.selfRefresh            = data.bit(7);
    break;

  //MSAR2
  case 0x5c:
    cs2.address.bit(16,23) = data;
    break;

  //MAMR2
  case 0x5d:
    cs2.mask.bit(15,22) = data;
    break;

  //MSAR3
  case 0x5e:
    cs3.address.bit(16,23) = data;
    break;

  //MAMR3
  case 0x5f:
    cs3.mask.bit(15,22) = data;
    break;

  //ADREG04L (read-only)
  case 0x60: break;

  //ADREG04H (read-only)
  case 0x61: break;

  //ADREG15L (read-only)
  case 0x62: break;

  //ADREG15H (read-only)
  case 0x63: break;

  //ADREG2L (read-only)
  case 0x64: break;

  //ADREG2H (read-only)
  case 0x65: break;

  //ADREG3L (read-only)
  case 0x66: break;

  //ADREG3H (read-only)
  case 0x67: break;

  //B0CS
  case 0x68:
    cs0.timing = data.bit(0,1);
    cs0.width  = data.bit(2) ? Byte : Word;
    cs0.enable = data.bit(4);
    break;

  //B1CS
  case 0x69:
    cs1.timing = data.bit(0,1);
    cs1.width  = data.bit(2) ? Byte : Word;
    cs1.enable = data.bit(4);
    break;

  //B2CS
  case 0x6a:
    cs2.timing = data.bit(0,1);
    cs2.width  = data.bit(2) ? Byte : Word;
    cs2.mode   = data.bit(3);
    cs2.enable = data.bit(4);
    break;

  //B3CS
  case 0x6b:
    cs3.timing = data.bit(0,1);
    cs3.width  = data.bit(2) ? Byte : Word;
    cs3.cas    = data.bit(3);
    cs3.enable = data.bit(4);
    break;

  //BEXCS
  case 0x6c:
    csx.timing = data.bit(0,1);
    csx.width  = data.bit(2) ? Byte : Word;
    break;

  //ADMOD
  case 0x6d: {
    n1 busy     = adc.busy;
    adc.channel = data.bit(0,1);
    n1 start    = data.bit(2);
    adc.speed   = data.bit(3);
    adc.scan    = data.bit(4);
    adc.repeat  = data.bit(5);

    if(!busy && start) {
      adc.busy = 1;
      adc.counter = 0;
    }
  } break;

  //WDMOD
  case 0x6e:
    watchdog.drive     = data.bit(0);
    watchdog.reset     = data.bit(1);
    watchdog.standby   = data.bit(2,3);
    watchdog.warmup    = data.bit(4);
    watchdog.frequency = data.bit(5,6);
    watchdog.enable    = data.bit(7);
    if(watchdog.enable) watchdog.counter = 0;  //todo: is this only on 0->1 transitions?
    break;

  //WDCR
  case 0x6f:
    if(data == 0x4e) watchdog.counter = 0;
    if(data == 0xb1) watchdog.enable  = 0;
    break;

  //INTE0AD
  case 0x70:
    int0.setPriority(data.bit(0,2));
    if(!data.bit(3)) int0.clear();
    intad.setPriority(data.bit(4,6));
    if(!data.bit(7)) intad.clear();
    break;

  //INTE45
  case 0x71:
    int4.setPriority(data.bit(0,2));
    if(!data.bit(3)) int4.clear();
    int5.setPriority(data.bit(4,6));
    if(!data.bit(7)) int5.clear();
    break;

  //INTE67
  case 0x72:
    int6.setPriority(data.bit(0,2));
    if(!data.bit(3)) int6.clear();
    int7.setPriority(data.bit(4,6));
    if(!data.bit(7)) int7.clear();
    break;

  //INTET01
  case 0x73:
    intt0.setPriority(data.bit(0,2));
    if(!data.bit(3)) intt0.clear();
    intt1.setPriority(data.bit(4,6));
    if(!data.bit(7)) intt1.clear();
    break;

  //INTET23
  case 0x74:
    intt2.setPriority(data.bit(0,2));
    if(!data.bit(3)) intt2.clear();
    intt3.setPriority(data.bit(4,6));
    if(!data.bit(7)) intt3.clear();
    break;

  //INTET45
  case 0x75:
    inttr4.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttr4.clear();
    inttr5.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttr5.clear();
    break;

  //INTET67
  case 0x76:
    inttr6.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttr6.clear();
    inttr7.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttr7.clear();
    break;

  //INTE50
  case 0x77:
    intrx0.setPriority(data.bit(0,2));
    if(!data.bit(3)) intrx0.clear();
    inttx0.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttx0.clear();
    break;

  //INTE51
  case 0x78:
    intrx1.setPriority(data.bit(0,2));
    if(!data.bit(3)) intrx1.clear();
    inttx1.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttx1.clear();
    break;

  //INTETC01
  case 0x79:
    inttc0.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttc0.clear();
    inttc1.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttc1.clear();
    break;

  //INTETC23
  case 0x7a:
    inttc2.setPriority(data.bit(0,2));
    if(!data.bit(3)) inttc2.clear();
    inttc3.setPriority(data.bit(4,6));
    if(!data.bit(7)) inttc3.clear();
    break;

  //IIMC
  case 0x7b:
    nmi.edge.rising  =  data.bit(0);
    int0.edge.rising = !data.bit(1);
    int0.level.high  =  data.bit(1);
    int0.setEnable(data.bit(2));
    interrupts.poll();
    break;

  //DMA0V
  case 0x7c:
    dma0.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    break;

  //DMA1V
  case 0x7d:
    dma1.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    break;

  //DMA2V
  case 0x7e:
    dma2.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    break;

  //DMA3V
  case 0x7f:
    dma3.vector.bit(2,6) = data.bit(0,4);
    interrupts.poll();
    break;

  case 0x80:
    clock.rate = data.bit(0,2);
    break;

  case 0x90: rtc.enable  = data.bit(0); break;
  case 0x91: rtc.year    = data; break;
  case 0x92: rtc.month   = data; break;
  case 0x93: rtc.day     = data; break;
  case 0x94: rtc.hour    = data; break;
  case 0x95: rtc.minute  = data; break;
  case 0x96: rtc.second  = data; break;
  case 0x97: rtc.weekday = data.bit(0,3); break;

  case 0xa0:
    psg.writeRight(data);
    break;

  case 0xa1:
    psg.writeLeft(data);
    break;

  case 0xa2:
    psg.writeRightDAC(data);
    break;

  case 0xa3:
    psg.writeLeftDAC(data);
    break;

  case 0xb2:
    misc.rtsDisable = data.bit(0);
    break;

  case 0xb3:
    nmi.setEnable(data.bit(2));
    break;

  case 0xb4:
    unknown.b4 = data;
    break;

  case 0xb5:
    unknown.b5 = data;
    break;

  case 0xb6:
    unknown.b6 = data;
    break;

  case 0xb7:
    unknown.b7 = data;
    break;

  case 0xb8:
    if(data == 0x55) psg.enablePSG();
    if(data == 0xaa) psg.enableDAC();
    break;

  case 0xb9:
    if(data == 0x55) apu.enable();
    if(data == 0xaa) apu.disable();
    break;

  case 0xba:
    apu.nmi.line = 1;
    break;

  case 0xbc:
    apu.port.data = data;
    break;
  }

  debugger.writeIO(address, data);
  return;
}
