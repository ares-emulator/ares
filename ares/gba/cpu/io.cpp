auto CPU::readIO(n32 address) -> n8 {
  auto dma = [&]() -> DMA& { return this->dma[address / 12 & 3]; };
  auto timer = [&]() -> Timer& { return this->timer[address.bit(2,3)]; };

  switch(address) {

  //DMA0CNT_L, DMA1CNT_L, DMA2CNT_L, DMA3CNT_L
  case 0x0400'00b8: case 0x0400'00c4: case 0x0400'00d0: case 0x0400'00dc: return 0x00;
  case 0x0400'00b9: case 0x0400'00c5: case 0x0400'00d1: case 0x0400'00dd: return 0x00;

  //DMA0CNT_H, DMA1CNT_H, DMA2CNT_H, DMA3CNT_H
  case 0x0400'00ba: case 0x0400'00c6: case 0x0400'00d2: case 0x0400'00de: return (
    dma().targetMode        << 5
  | dma().sourceMode.bit(0) << 7
  );
  case 0x0400'00bb: case 0x0400'00c7: case 0x0400'00d3: case 0x0400'00df: return (
    dma().sourceMode.bit(1) << 0
  | dma().repeat            << 1
  | dma().size              << 2
  | dma().drq               << 3
  | dma().timingMode        << 4
  | dma().irq               << 6
  | dma().enable            << 7
  );

  //TM0CNT_L, TM1CNT_L, TM2CNT_L, TM3CNT_L
  case 0x0400'0100: case 0x0400'0104: case 0x0400'0108: case 0x0400'010c: return timer().period.byte(0);
  case 0x0400'0101: case 0x0400'0105: case 0x0400'0109: case 0x0400'010d: return timer().period.byte(1);

  //TM0CNT_H, TM1CNT_H, TM2CNT_H, TM3CNT_H
  case 0x0400'0102: case 0x0400'0106: case 0x0400'010a: case 0x0400'010e: return (
    timer().frequency << 0
  | timer().cascade   << 2
  | timer().irq       << 6
  | timer().enable    << 7
  );
  case 0x0400'0103: case 0x0400'0107: case 0x0400'010b: case 0x0400'010f: return 0;

  //SIOMULTI0 (SIODATA32_L), SIOMULTI1 (SIODATA32_H), SIOMULTI2, SIOMULTI3
  case 0x0400'0120: case 0x0400'0122: case 0x0400'0124: case 0x0400'0126: {
    if(auto data = player.read()) return data().byte(address.bit(0,1));
    return serial.data[address.bit(1,2)].byte(0);
  }
  case 0x0400'0121: case 0x0400'0123: case 0x0400'0125: case 0x0400'0127: {
    if(auto data = player.read()) return data().byte(address.bit(0,1));
    return serial.data[address.bit(1,2)].byte(1);
  }

  //SIOCNT
  case 0x0400'0128: return (
    serial.shiftClockSelect      << 0
  | serial.shiftClockFrequency   << 1
  | serial.transferEnableReceive << 2
  | serial.transferEnableSend    << 3
  | serial.startBit              << 7
  );
  case 0x0400'0129: return (
    serial.transferLength << 4
  | serial.irqEnable      << 6
  );

  //SIOMLT_SEND (SIODATA8)
  case 0x0400'012a: return serial.data8;
  case 0x0400'012b: return 0;

  //KEYINPUT
  case 0x04000130: {
    if(Model::GameBoyPlayer()) {
      if(auto result = player.keyinput()) return result() >> 0;
    }
    system.controls.poll();
    n8 result;
    result.bit(0) = !system.controls.a->value();
    result.bit(1) = !system.controls.b->value();
    result.bit(2) = !system.controls.select->value();
    result.bit(3) = !system.controls.start->value();
    if(ppu.rotation->value() == "0°") {
      result.bit(4) = !system.controls.rightLatch;
      result.bit(5) = !system.controls.leftLatch;
      result.bit(6) = !system.controls.upLatch;
      result.bit(7) = !system.controls.downLatch;
    }
    if(ppu.rotation->value() == "90°") {
      result.bit(4) = !system.controls.downLatch;
      result.bit(5) = !system.controls.upLatch;
      result.bit(6) = !system.controls.rightLatch;
      result.bit(7) = !system.controls.leftLatch;
    }
    if(ppu.rotation->value() == "180°") {
      result.bit(4) = !system.controls.leftLatch;
      result.bit(5) = !system.controls.rightLatch;
      result.bit(6) = !system.controls.downLatch;
      result.bit(7) = !system.controls.upLatch;
    }
    if(ppu.rotation->value() == "270°") {
      result.bit(4) = !system.controls.upLatch;
      result.bit(5) = !system.controls.downLatch;
      result.bit(6) = !system.controls.leftLatch;
      result.bit(7) = !system.controls.rightLatch;
    }
    return result;
  }
  case 0x04000131: {
    if(Model::GameBoyPlayer()) {
      if(auto result = player.keyinput()) return result() >> 8;
    }
    system.controls.poll();
    n8 result;
    result.bit(0) = !system.controls.r->value();
    result.bit(1) = !system.controls.l->value();
    return result;
  }

  //KEYCNT
  case 0x0400'0132: return (
    keypad.flag[0] << 0
  | keypad.flag[1] << 1
  | keypad.flag[2] << 2
  | keypad.flag[3] << 3
  | keypad.flag[4] << 4
  | keypad.flag[5] << 5
  | keypad.flag[6] << 6
  | keypad.flag[7] << 7
  );
  case 0x0400'0133: return (
    keypad.flag[8]   << 0
  | keypad.flag[9]   << 1
  | keypad.enable    << 6
  | keypad.condition << 7
  );

  //RCNT
  case 0x0400'0134: return (
    joybus.sc     << 0
  | joybus.sd     << 1
  | joybus.si     << 2
  | joybus.so     << 3
  | joybus.scMode << 4
  | joybus.sdMode << 5
  | joybus.siMode << 6
  | joybus.soMode << 7
  );
  case 0x0400'0135: return (
    joybus.siIRQEnable << 0
  | joybus.mode        << 6
  );
  
  //zero
  case 0x0400'0136: return 0;
  case 0x0400'0137: return 0;

  //JOYCNT
  case 0x0400'0140: return (
    joybus.resetSignal     << 0
  | joybus.receiveComplete << 1
  | joybus.sendComplete    << 2
  | joybus.resetIRQEnable  << 6
  );
  case 0x0400'0141: return 0;
  case 0x0400'0142: return 0;
  case 0x0400'0143: return 0;

  //JOY_RECV_L, JOY_RECV_H
  case 0x0400'0150: return joybus.receive.byte(0);
  case 0x0400'0151: return joybus.receive.byte(1);
  case 0x0400'0152: return joybus.receive.byte(2);
  case 0x0400'0153: return joybus.receive.byte(3);

  //JOY_TRANS_L, JOY_TRANS_H
  case 0x0400'0154: return joybus.transmit.byte(0);
  case 0x0400'0155: return joybus.transmit.byte(1);
  case 0x0400'0156: return joybus.transmit.byte(2);
  case 0x0400'0157: return joybus.transmit.byte(3);

  //JOYSTAT
  case 0x0400'0158: return (
    joybus.receiveFlag << 1
  | joybus.sendFlag    << 3
  | joybus.generalFlag << 4
  );
  case 0x0400'0159: return 0;
  case 0x0400'015a: return 0;
  case 0x0400'015b: return 0;

  //IE
  case 0x0400'0200: return irq.enable.byte(0);
  case 0x0400'0201: return irq.enable.byte(1);

  //IF
  case 0x0400'0202: return irq.flag.byte(0);
  case 0x0400'0203: return irq.flag.byte(1);

  //WAITCNT
  case 0x0400'0204: return (
    wait.nwait[3] << 0
  | wait.nwait[0] << 2
  | wait.swait[0] << 4
  | wait.nwait[1] << 5
  | wait.swait[1] << 7
  );
  case 0x0400'0205: return (
    wait.nwait[2] << 0
  | wait.swait[2] << 2
  | wait.phi      << 3
  | wait.prefetch << 6
  | wait.gameType << 7
  );
  
  //zero
  case 0x0400'0206: return 0;
  case 0x0400'0207: return 0;

  //IME
  case 0x0400'0208: return irq.ime;
  case 0x0400'0209: return 0;
  
  //zero
  case 0x0400'020a: return 0;
  case 0x0400'020b: return 0;

  //POSTFLG + HALTCNT
  case 0x0400'0300: return context.booted;
  case 0x0400'0301: return 0;
  
  //zero
  case 0x0400'0302: return 0;
  case 0x0400'0303: return 0;

  //MEMCNT_L
  case 0x0400'0800: return (
    memory.biosSwap << 0
  | memory.unknown1 << 1
  | memory.ewram    << 5
  );
  case 0x0400'0801: return 0;

  //MEMCNT_H
  case 0x0400'0802: return 0;
  case 0x0400'0803: return (
    memory.ewramWait << 0
  | memory.unknown2  << 4
  );

  }

  if(cpu.context.dmaActive) return cpu.dmabus.data.byte(address & 3);
  return cpu.pipeline.fetch.instruction.byte(address & 1);
}

auto CPU::writeIO(n32 address, n8 data) -> void {
  auto dma = [&]() -> DMA& { return this->dma[address / 12 & 3]; };
  auto timer = [&]() -> Timer& { return this->timer[address.bit(2,3)]; };

  switch(address) {

  //DMA0SAD, DMA1SAD, DMA2SAD, DMA3SAD
  case 0x0400'00b0: case 0x0400'00bc: case 0x0400'00c8: case 0x0400'00d4: dma().source.data.byte(0) = data; return;
  case 0x0400'00b1: case 0x0400'00bd: case 0x0400'00c9: case 0x0400'00d5: dma().source.data.byte(1) = data; return;
  case 0x0400'00b2: case 0x0400'00be: case 0x0400'00ca: case 0x0400'00d6: dma().source.data.byte(2) = data; return;
  case 0x0400'00b3: case 0x0400'00bf: case 0x0400'00cb: case 0x0400'00d7: dma().source.data.byte(3) = data; return;

  //DMA0DAD, DMA1DAD, DMA2DAD, DMA3DAD
  case 0x0400'00b4: case 0x0400'00c0: case 0x0400'00cc: case 0x0400'00d8: dma().target.data.byte(0) = data; return;
  case 0x0400'00b5: case 0x0400'00c1: case 0x0400'00cd: case 0x0400'00d9: dma().target.data.byte(1) = data; return;
  case 0x0400'00b6: case 0x0400'00c2: case 0x0400'00ce: case 0x0400'00da: dma().target.data.byte(2) = data; return;
  case 0x0400'00b7: case 0x0400'00c3: case 0x0400'00cf: case 0x0400'00db: dma().target.data.byte(3) = data; return;

  //DMA0CNT_L, DMA1CNT_L, DMA2CNT_L, DMA3CNT_L
  case 0x0400'00b8: case 0x0400'00c4: case 0x0400'00d0: case 0x0400'00dc: dma().length.data.byte(0) = data; return;
  case 0x0400'00b9: case 0x0400'00c5: case 0x0400'00d1: case 0x0400'00dd: dma().length.data.byte(1) = data; return;

  //DMA0CNT_H, DMA1CNT_H, DMA2CNT_H, DMA3CNT_H
  case 0x0400'00ba: case 0x0400'00c6: case 0x0400'00d2: case 0x0400'00de:
    dma().targetMode        = data.bit(5,6);
    dma().sourceMode.bit(0) = data.bit(7);
    return;
  case 0x0400'00bb: case 0x0400'00c7: case 0x0400'00d3: case 0x0400'00df: {
    bool enable = dma().enable;
    if(address != 0x0400'00df) data.bit(3) = 0;  //gamepad DRQ valid for DMA3 only

    dma().sourceMode.bit(1) = data.bit(0);
    dma().repeat            = data.bit(1);
    dma().size              = data.bit(2);
    dma().drq               = data.bit(3);
    dma().timingMode        = data.bit(4,5);
    dma().irq               = data.bit(6);
    dma().enable            = data.bit(7);

    if(!enable && dma().enable) {  //0->1 transition
      if(dma().timingMode == 0) {
        dma().active = true;  //immediate transfer mode
        dma().waiting = 2;
      }
      dma().latch.source = dma().source;
      dma().latch.target = dma().target;
      dma().latch.length = dma().length;
    } else if(!dma().enable) {
      dma().active = false;
    }
    return;
  }

  //TM0CNT_L, TM1CNT_L, TM2CNT_L, TM3CNT_L
  case 0x0400'0100: case 0x0400'0104: case 0x0400'0108: case 0x0400'010c: timer().reload.byte(0) = data; return;
  case 0x0400'0101: case 0x0400'0105: case 0x0400'0109: case 0x0400'010d: timer().reload.byte(1) = data; return;

  //TM0CNT_H, TM1CNT_H, TM2CNT_H, TM3CNT_H
  case 0x0400'0102: case 0x0400'0106: case 0x0400'010a: case 0x0400'010e: {
    bool enable = timer().enable;

    timer().frequency = data.bit(0,1);
    timer().irq       = data.bit(6);
    timer().enable    = data.bit(7);

    if(address != 0x0400'0102) timer().cascade = data.bit(2);

    if(!enable && timer().enable) {  //0->1 transition
      timer().pending = true;
    }
    return;
  }
  case 0x0400'0103: case 0x0400'0107: case 0x0400'010b: case 0x0400'010f:
    return;

  //SIOMULTI0 (SIODATA32_L), SIOMULTI1 (SIODATA32_H), SIOMULTI2, SIOMULTI3
  case 0x0400'0120: case 0x0400'0122: case 0x0400'0124: case 0x0400'0126:
    player.write(address.bit(0,1), data);
    serial.data[address.bit(1,2)].byte(0) = data;
    return;
  case 0x0400'0121: case 0x0400'0123: case 0x0400'0125: case 0x0400'0127:
    player.write(address.bit(0,1), data);
    serial.data[address.bit(1,2)].byte(1) = data;
    return;

  //SIOCNT
  case 0x0400'0128:
    serial.shiftClockSelect      = data.bit(0);
    serial.shiftClockFrequency   = data.bit(1);
    serial.transferEnableReceive = data.bit(2);
    serial.transferEnableSend    = data.bit(3);
    serial.startBit              = data.bit(7);
    return;
  case 0x0400'0129:
    serial.transferLength = data.bit(4);
    serial.irqEnable      = data.bit(6);
    return;

  //SIOMLT_SEND (SIODATA8)
  case 0x0400'012a: serial.data8 = data; return;
  case 0x0400'012b: return;

  //KEYCNT
  case 0x0400'0132:
    keypad.flag[0] = data.bit(0);
    keypad.flag[1] = data.bit(1);
    keypad.flag[2] = data.bit(2);
    keypad.flag[3] = data.bit(3);
    keypad.flag[4] = data.bit(4);
    keypad.flag[5] = data.bit(5);
    keypad.flag[6] = data.bit(6);
    keypad.flag[7] = data.bit(7);
    return;
  case 0x0400'0133:
    keypad.flag[8]   = data.bit(0);
    keypad.flag[9]   = data.bit(1);
    keypad.enable    = data.bit(6);
    keypad.condition = data.bit(7);
    return;

  //RCNT
  case 0x0400'0134:
    joybus.sc     = data.bit(0);
    joybus.sd     = data.bit(1);
    joybus.si     = data.bit(2);
    joybus.so     = data.bit(3);
    joybus.scMode = data.bit(4);
    joybus.sdMode = data.bit(5);
    joybus.siMode = data.bit(6);
    joybus.soMode = data.bit(7);
    return;
  case 0x0400'0135:
    joybus.siIRQEnable = data.bit(0);
    joybus.mode        = data.bit(6,7);
    return;

  //JOYCNT
  case 0x0400'0140:
    joybus.resetSignal     = data.bit(0);
    joybus.receiveComplete = data.bit(1);
    joybus.sendComplete    = data.bit(2);
    joybus.resetIRQEnable  = data.bit(6);
    return;
  case 0x0400'0141: return;
  case 0x0400'0142: return;
  case 0x0400'0143: return;

  //JOY_RECV_L
  //JOY_RECV_H
  case 0x0400'0150: joybus.receive.byte(0) = data; return;
  case 0x0400'0151: joybus.receive.byte(1) = data; return;
  case 0x0400'0152: joybus.receive.byte(2) = data; return;
  case 0x0400'0153: joybus.receive.byte(3) = data; return;

  //JOY_TRANS_L
  //JOY_TRANS_H
  case 0x0400'0154: joybus.transmit.byte(0) = data; return;
  case 0x0400'0155: joybus.transmit.byte(1) = data; return;
  case 0x0400'0156: joybus.transmit.byte(2) = data; return;
  case 0x0400'0157: joybus.transmit.byte(3) = data; return;

  //JOYSTAT
  case 0x0400'0158:
    joybus.receiveFlag = data.bit(1);
    joybus.sendFlag    = data.bit(3);
    joybus.generalFlag = data.bit(4,5);
    return;
  case 0x0400'0159: return;

  //IE
  case 0x0400'0200: irq.enable.byte(0) = data; return;
  case 0x0400'0201: irq.enable.byte(1) = data; return;

  //IF
  case 0x0400'0202: irq.flag.byte(0) = irq.flag.byte(0) & ~data; return;
  case 0x0400'0203: irq.flag.byte(1) = irq.flag.byte(1) & ~data; return;

  //WAITCNT
  case 0x0400'0204:
    wait.swait[3] = data.bit(0);  //todo: is this correct?
    wait.nwait[3] = data.bit(0,1);
    wait.nwait[0] = data.bit(2,3);
    wait.swait[0] = data.bit(4);
    wait.nwait[1] = data.bit(5,6);
    wait.swait[1] = data.bit(7);
    return;
  case 0x0400'0205:
    wait.nwait[2] = data.bit(0,1);
    wait.swait[2] = data.bit(2);
    wait.phi      = data.bit(3);
    wait.prefetch = data.bit(6);
  //wait.gameType is read-only
    return;

  //IME
  case 0x0400'0208: irq.ime = data.bit(0); return;
  case 0x0400'0209: return;

  //POSTFLG, HALTCNT
  case 0x0400'0300:
    if(data.bit(0)) context.booted = 1;
    return;
  case 0x0400'0301:
    context.halted  = data.bit(7) == 0;
    context.stopped = data.bit(7) == 1;
    return;

  //MEMCNT_L
  //MEMCNT_H
  case 0x0400'0800:
    memory.biosSwap = data.bit(0);
    memory.unknown1 = data.bit(1,3);
    memory.ewram    = data.bit(5);
    return;
  case 0x0400'0801: return;
  case 0x0400'0802: return;
  case 0x0400'0803:
    memory.ewramWait = data.bit(0,3);
    memory.unknown2  = data.bit(4,7);
    return;

  }
}
