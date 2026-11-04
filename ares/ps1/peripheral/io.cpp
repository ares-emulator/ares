auto Peripheral::receive() -> u8 {
  u8 data = io.receiveData;
  io.receiveData >>= 8;
  if(io.receiveSize) io.receiveSize--;
  return data;
}

auto Peripheral::transmit(u8 data) -> void {
  io.transmitData = data;
  io.transmitPending = 1;
  io.transmitEnableLatched = io.transmitEnable;
  io.transmitStarted = 0;
  startTransfer();
}

auto Peripheral::startTransfer() -> void {
  if(io.transferElapses || !io.transmitPending) return;
  if(!io.transmitEnable && !io.transmitEnableLatched) return;

  u8 data = io.transmitData;
  io.transmitPending = 0;
  io.transmitFinished = 0;

  io.transferElapses = 2 * (5 + io.characterLength + io.parityEnable);

  auto transfer = [&](PeripheralPort& port) {
    io.receiveShift = port.bus(data);
    if(u32 clocks = port.acknowledge()) io.ackDelayCounter = clocks;
  };

  auto& controller = io.slotNumber ? controllerPort2 : controllerPort1;
  auto& memoryCard = io.slotNumber ? memoryCardPort2 : memoryCardPort1;
  io.receiveShift = 0xff;
  if(io.joyOutput) {
    if(!memoryCard.active()) transfer(controller);
    if(!controller.active()) transfer(memoryCard);
  }
  io.receiveShift &= (1u << (5 + io.characterLength)) - 1;
  if(u32 clocks = nextEvent()) cpu.setNextEvent(clocks);
}

auto Peripheral::readByte(u32 address) -> u32 {
  n8 data;

  debug(unhandled, "Peripheral::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto Peripheral::readHalf(u32 address) -> u32 {
  n16 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    data = (u16)io.receiveData;
    receive();
    return data;
  }

  //JOY_STAT
  if(address == 0x1f80'1044) {
    data.bit(0) = io.transmitStarted;
    data.bit(1) = io.receiveSize > 0;
    data.bit(2) = io.transmitFinished;
    data.bit(3) = io.parityError;
    data.bit(7) = io.acknowledgeAsserted;
    data.bit(9) = io.interruptRequest;
    data.bit(11,15) = io.baudrateTimer & 0x1f;
    return data;
  }

  if(address == 0x1f80'1046) return io.baudrateTimer >> 5 & 0xffff;

  //JOY_MODE
  if(address == 0x1f80'1048) {
    data.bit(0, 1) = io.baudrateReloadFactor;
    data.bit(2, 3) = io.characterLength;
    data.bit(4)    = io.parityEnable;
    data.bit(5)    = io.parityType;
    data.bit(8)    = io.clockOutputPolarity;
    return data;
  }

  //JOY_CTRL
  if(address == 0x1f80'104a) {
    data.bit( 0)    = io.transmitEnable;
    data.bit( 1)    = io.joyOutput;
    data.bit( 2)    = io.receiveEnable;
    data.bit( 3)    = io.unknownCtrl_3;
    data.bit( 5)    = io.unknownCtrl_5;
    data.bit( 8, 9) = io.receiveInterruptMode;
    data.bit(10)    = io.transmitInterruptEnable;
    data.bit(11)    = io.receiveInterruptEnable;
    data.bit(12)    = io.acknowledgeInterruptEnable;
    data.bit(13)    = io.slotNumber;
    return data;
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) return io.baudrateReloadValue;

  if(address == 0x1f80'1042 || address == 0x1f80'104c) return 0;

  if(address == 0x1f80'1050 || address == 0x1f80'1052 || address == 0x1f80'1054 || address == 0x1f80'1056) return 0;

  if(address == 0x1f80'1058) return io.sioMode;

  if(address == 0x1f80'105a) return io.sioControl & 0x1faf;

  if(address == 0x1f80'105c) return io.sioInternal;

  if(address == 0x1f80'105e) return io.sioBaudrateReloadValue;

  debug(unhandled, "Peripheral::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto Peripheral::readWord(u32 address) -> u32 {
  n32 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    data = (u32)io.receiveData;
    for(u32 byte : range(4)) receive();
    return data;
  }

  if(address >= 0x1f80'1044 && address <= 0x1f80'105c) {
    return readHalf(address) | readHalf(address + 2) << 16;
  }

  debug(unhandled, "Peripheral::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto Peripheral::writeByte(u32 address, u32 value) -> void {
  n8 data = value;

  debug(unhandled, "Peripheral::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto Peripheral::writeHalf(u32 address, u32 value) -> void {
  n16 data = value;

  //JOY_TX_DATA
  if(address == 0x1f80'1040) {
    transmit(data);
    return;
  }

  //JOY_MODE
  if(address == 0x1f80'1048) {
    io.baudrateReloadFactor = data.bit(0, 1);
    io.characterLength      = data.bit(2, 3);
    io.parityEnable         = data.bit(4);
    io.parityType           = data.bit(5);
    io.clockOutputPolarity  = data.bit(8);
    return;
  }

  //JOY_CTRL
  if(address == 0x1f80'104a) {
    bool acknowledge = data.bit(4);
    bool reset = data.bit(6);
    io.transmitEnable             = data.bit( 0);
    io.joyOutput                  = data.bit( 1);
    io.receiveEnable              = data.bit( 2);
    io.unknownCtrl_3              = data.bit( 3);
    io.unknownCtrl_5              = data.bit( 5);
    io.receiveInterruptMode       = data.bit( 8, 9);
    io.transmitInterruptEnable    = data.bit(10);
    io.receiveInterruptEnable     = data.bit(11);
    io.acknowledgeInterruptEnable = data.bit(12);
    io.slotNumber                 = data.bit(13);

    if(!io.joyOutput) {
      controllerPort1.reset();
      memoryCardPort1.reset();
      controllerPort2.reset();
      memoryCardPort2.reset();
    }

    if(acknowledge || reset) {
      io.parityError = 0;
      io.interruptRequest = 0;
      interrupt.lower(Interrupt::Peripheral);
    }

    if(reset) {
      io.transmitEnable = 0;
      io.joyOutput = 0;
      io.receiveEnable = 0;
      io.unknownCtrl_3 = 0;
      io.unknownCtrl_5 = 0;
      io.receiveInterruptMode = 0;
      io.transmitInterruptEnable = 0;
      io.receiveInterruptEnable = 0;
      io.acknowledgeInterruptEnable = 0;
      io.slotNumber = 0;

      io.baudrateReloadFactor = 0;
      io.characterLength = 0;
      io.parityEnable = 0;
      io.parityType = 0;
      io.clockOutputPolarity = 0;

      io.baudrateReloadValue = 0;
      io.receiveSize = 0;
      io.transmitPending = 0;
      io.transmitEnableLatched = 0;
      io.transmitStarted = 1;
      io.transmitFinished = 1;
      io.transferElapses = 0;
      io.ackDelayCounter = 0;
      io.ackAssertCounter = 0;
      io.acknowledgeAsserted = 0;

      controllerPort1.reset();
      memoryCardPort1.reset();
      controllerPort2.reset();
      memoryCardPort2.reset();
    }
    startTransfer();
    pollInterrupt();
    return;
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    io.baudrateReloadValue = data.bit(0,15);
    io.baudrateTimer = baudrateReload();
    return;
  }

  if(address == 0x1f80'1042 || address == 0x1f80'1044 || address == 0x1f80'1046 || address == 0x1f80'104c) return;

  if(address == 0x1f80'1050 || address == 0x1f80'1052 || address == 0x1f80'1054 || address == 0x1f80'1056) return;

  if(address == 0x1f80'1058) { io.sioMode = data.bit(0,7); return; }

  if(address == 0x1f80'105a) {
    io.sioControl = data & 0x1faf;
    if(data.bit(6)) {
      io.sioControl = 0;
      io.sioMode = 0;
      io.sioBaudrateReloadValue = 0;
    }
    return;
  }

  if(address == 0x1f80'105c) { io.sioInternal = data; return; }

  if(address == 0x1f80'105e) { io.sioBaudrateReloadValue = data; return; }

  debug(unhandled, "Peripheral::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto Peripheral::writeWord(u32 address, u32 data) -> void {
  writeHalf(address, data);
}
