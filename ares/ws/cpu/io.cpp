auto CPU::keypadRead() -> n4 {
  n4 data;
  bool horizontal = ppu.screen->rotation() == 0;

  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    if(r.keypadMatrix.bit(0)) {  //d4
      if(horizontal) {
        data.bit(0) = system.controls.y1->value();
        data.bit(1) = system.controls.y2->value();
        data.bit(2) = system.controls.y3->value();
        data.bit(3) = system.controls.y4->value();
      } else {
        data.bit(0) = system.controls.x4->value();
        data.bit(1) = system.controls.x1->value();
        data.bit(2) = system.controls.x2->value();
        data.bit(3) = system.controls.x3->value();
      }
    }

    if(r.keypadMatrix.bit(1)) {  //d5
      if(horizontal) {
        data.bit(0) = system.controls.x1->value();
        data.bit(1) = system.controls.x2->value();
        data.bit(2) = system.controls.x3->value();
        data.bit(3) = system.controls.x4->value();
      } else {
        data.bit(0) = system.controls.y4->value();
        data.bit(1) = system.controls.y1->value();
        data.bit(2) = system.controls.y2->value();
        data.bit(3) = system.controls.y3->value();
      }
    }

    if(r.keypadMatrix.bit(2)) {  //d6
      data.bit(1) = system.controls.start->value();
      data.bit(2) = system.controls.a->value();
      data.bit(3) = system.controls.b->value();
    }
  }

  if(Model::PocketChallengeV2()) {
    //this pin is always forced to logic high, which has the practical effect of bypassing the IPLROM.
    data.bit(1) = 1;

    if(r.keypadMatrix.bit(0)) {  //d4
      data.bit(0) = system.controls.clear->value();
      data.bit(2) = system.controls.circle->value();
      data.bit(3) = system.controls.pass->value();
    }

    if(r.keypadMatrix.bit(1)) {  //d5
      data.bit(0) = system.controls.view->value();
      data.bit(2) = system.controls.escape->value();
      data.bit(3) = system.controls.rightLatch;
    }

    if(r.keypadMatrix.bit(2)) {  //d6
      data.bit(0) = system.controls.leftLatch;
      data.bit(2) = system.controls.down->value();
      data.bit(3) = system.controls.up->value();
    }
  }

  return data;
}

auto CPU::portRead(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x0040 ... 0x0042:  //DMA_SRC
    data = r.dmaSource.byte(address - 0x0040);
    break;

  case 0x0044 ... 0x0045:  //DMA_DST
    data = r.dmaTarget.byte(address - 0x0044);
    break;

  case 0x0046 ... 0x0047:  //DMA_LEN
    data = r.dmaLength.byte(address - 0x0046);
    break;

  case 0x0048:  //DMA_CTRL
    data.bit(0) = r.dmaMode;
    data.bit(7) = r.dmaEnable;
    break;

  case 0x0062:  //WSC_SYSTEM
    data.bit(7) = SoC::SPHINX2();
    break;

  case 0x00a0:  //HW_FLAGS
    data.bit(0) = r.cartridgeEnable;
    data.bit(1) = !SoC::ASWAN();
    data.bit(2) = 1;  //0 = 8-bit bus width; 1 = 16-bit bus width
    data.bit(7) = 1;  //1 = built-in self-test passed
    break;

  case 0x00b0:  //INT_BASE
    data  = r.interruptBase;
    data |= SoC::ASWAN() ? 3 : 0;
    break;

  case 0x00b1:  //SER_DATA
    data = r.serialData;
    break;

  case 0x00b2:  //INT_ENABLE
    data = r.interruptEnable;
    break;

  case 0x00b3:  //SER_STATUS
    data.bit(2) = 1;  //hack: always report send buffer as empty
    data.bit(6) = r.serialBaudRate;
    data.bit(7) = r.serialEnable;
    break;

  case 0x00b4:  //INT_STATUS
    data = r.interruptStatus;
    break;

  case 0x00b5:  //KEYPAD
    data.bit(0,3) = keypadRead();
    data.bit(4,6) = r.keypadMatrix;
    break;

  }

  return data;
}

auto CPU::portWrite(n16 address, n8 data) -> void {
  switch(address) {

  case 0x0040 ... 0x0042:  //DMA_SRC
    r.dmaSource.byte(address - 0x0040) = data;
    r.dmaSource &= ~1;
    break;

  case 0x0044 ... 0x0045:  //DMA_DST
    r.dmaTarget.byte(address - 0x0044) = data;
    r.dmaTarget &= ~1;
    break;

  case 0x0046 ... 0x0047:  //DMA_LEN
    r.dmaLength.byte(address - 0x0046) = data;
    r.dmaLength &= ~1;
    break;

  case 0x0048:  //DMA_CTRL
    r.dmaMode   = data.bit(6);
    r.dmaEnable = data.bit(7);
    if(r.dmaEnable) dmaTransfer();
    break;

  case 0x0062:  //WSC_SYSTEM
    if(!SoC::ASWAN()) {
      if(data.bit(0)) scheduler.exit(Event::Power);
    }
    break;

  case 0x00a0:  //HW_FLAGS
    //todo: d2 (bus width) bit is writable; but ... it will do very bad things
    r.cartridgeEnable |= data.bit(0);  //bit can never be unset (boot ROM lockout)
    break;

  case 0x00b0:  //INT_BASE
    r.interruptBase = SoC::ASWAN() ? data & ~7 : data & ~1;
    break;

  case 0x00b1:  //SER_DATA
    r.serialData = data;
    break;

  case 0x00b2:  //INT_ENABLE
    r.interruptEnable = data;
    r.interruptStatus &= ~r.interruptEnable;
    break;

  case 0x00b3:  //SER_STATUS
    r.serialBaudRate = data.bit(6);
    r.serialEnable   = data.bit(7);
    break;

  case 0x00b5:  //KEYPAD
    r.keypadMatrix = data.bit(4,6);
    break;

  case 0x00b6:  //INT_ACK
    //acknowledge only edge-sensitive interrupts
    r.interruptStatus &= ~(data & 0b11110010);
    break;

  }

  return;
}
