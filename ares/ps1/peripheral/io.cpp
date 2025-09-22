auto Peripheral::receive() -> u8 {
  u8 data = 0xff;
  if(io.receiveSize) {
    data = io.receiveData;
    io.receiveData = 0xff;
    io.receiveSize--;
  }
  return data;
}

auto Peripheral::transmit(u8 data) -> void {
  if(!io.joyOutput) return;

  if(io.ackCounter > 0) {
    debug(unusual, "Peripheral::transmit: ackCounter > 0");
    //HACK: Recover by deasserting /ack and /irq; we need to figure out why this happens
    //TODO: Why is transmit happening too early? Does real hardware do this?
    io.acknowledgeAsserted = 0;
    io.interruptRequest = 0;
  }

  //Calculate the number of cycles required to transfer a byte at the current baud rate
  //This is added to the /ACK delay to determine the total duration until /ACK is asserted
  u8 factors[4] = {1, 1, 16, 64};
  io.transferCounter = ((io.baudrateReloadValue * factors[io.baudrateReloadFactor])) * 8;

  if(io.slotNumber == 0) {
    if(!memoryCardPort1.active()) {
      io.receiveData = controllerPort1.bus(data);
      if(controllerPort1.acknowledge()) io.ackCounter = 338; // approx 9.98us
    }

    if(!controllerPort1.active()) {
      io.receiveData = memoryCardPort1.bus(data);
      if(memoryCardPort1.acknowledge()) io.ackCounter = 170; // approx 5us
    }
  }

  if(io.slotNumber == 1) {
    if(!memoryCardPort2.active()) {
      io.receiveData = controllerPort2.bus(data);
      if(controllerPort2.acknowledge()) io.ackCounter = 338; // approx 9.98us
    }

    if(!controllerPort2.active()) {
      io.receiveData = memoryCardPort2.bus(data);
      if(memoryCardPort2.acknowledge()) io.ackCounter = 170; // approx 5us
    }
  }
}

auto Peripheral::readByte(u32 address) -> u32 {
  n8 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    data = receive();
  }

  return data;
}

auto Peripheral::readHalf(u32 address) -> u32 {
  n16 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    data = receive();
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
    return data;
  }

  //JOY_MODE
  if(address == 0x1f80'1048) {
    data.bit(0, 1) = io.baudrateReloadFactor;
    data.bit(2, 3) = io.characterLength;
    data.bit(4)    = io.parityEnable;
    data.bit(5)    = io.parityType;
    data.bit(6, 7) = io.unknownMode_6_7;
    data.bit(8)    = io.clockOutputPolarity;
    data.bit(9,15) = io.unknownMode_9_15;
    return data;
  }

  //JOY_CTRL
  if(address == 0x1f80'104a) {
    data.bit( 0)    = io.transmitEnable;
    data.bit( 1)    = io.joyOutput;
    data.bit( 2)    = io.receiveEnable;
    data.bit( 3)    = io.unknownCtrl_3;
    data.bit( 4)    = io.acknowledge;
    data.bit( 5)    = io.unknownCtrl_5;
    data.bit( 6)    = io.reset;
    data.bit( 7)    = io.unknownCtrl_7;
    data.bit( 8, 9) = io.receiveInterruptMode;
    data.bit(10)    = io.transmitInterruptEnable;
    data.bit(11)    = io.receiveInterruptEnable;
    data.bit(12)    = io.acknowledgeInterruptEnable;
    data.bit(13)    = io.slotNumber;
    data.bit(14,15) = io.unknownCtrl_14_15;
    return data;
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    data.bit(0,15) = io.baudrateReloadValue;
    return data;
  }

  debug(unhandled, "Peripheral::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto Peripheral::readWord(u32 address) -> u32 {
  n32 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    if(io.receiveSize) {
      data = io.receiveData;
      io.receiveData = 0xff;
      io.receiveSize--;
    }
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
    return data;
  }

  debug(unhandled, "Peripheral::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto Peripheral::writeByte(u32 address, u32 value) -> void {
  n8 data = value;

  //JOY_TX_DATA
  if(address == 0x1f80'1040) {
    transmit(data);
    return;
  }

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
    io.unknownMode_6_7      = data.bit(6, 7);
    io.clockOutputPolarity  = data.bit(8);
    io.unknownMode_9_15     = data.bit(9,15);
    return;
  }

  //JOY_CTRL
  if(address == 0x1f80'104a) {
    io.transmitEnable             = data.bit( 0);
    io.joyOutput                  = data.bit( 1);
    io.receiveEnable              = data.bit( 2);
    io.unknownCtrl_3              = data.bit( 3);
    io.acknowledge                = data.bit( 4);
    io.unknownCtrl_5              = data.bit( 5);
    io.reset                      = data.bit( 6);
    io.unknownCtrl_7              = data.bit( 7);
    io.receiveInterruptMode       = data.bit( 8, 9);
    io.transmitInterruptEnable    = data.bit(10);
    io.receiveInterruptEnable     = data.bit(11);
    io.acknowledgeInterruptEnable = data.bit(12);
    io.slotNumber                 = data.bit(13);
    io.unknownCtrl_14_15          = data.bit(14,15);

    if(!io.joyOutput) {
      controllerPort1.reset();
      memoryCardPort1.reset();
      controllerPort2.reset();
      memoryCardPort2.reset();
    }

    if(io.acknowledge || io.reset) {
      io.parityError = 0;
      io.interruptRequest = 0;
      interrupt.lower(Interrupt::Peripheral);
    }
    return;
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    io.baudrateReloadValue = data.bit(0,15);
    return;
  }

  debug(unhandled, "Peripheral::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto Peripheral::writeWord(u32 address, u32 data) -> void {
  debug(unimplemented, "Peripheral::writeWord");
}
