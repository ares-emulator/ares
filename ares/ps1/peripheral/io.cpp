auto Peripheral::receive() -> u8 {
  u8 data = 0xff;
  if(io.receiveSize) {
    data = io.receiveData;
    io.receiveData >>= 8;
    io.receiveSize--;
  }
  return data;
}

auto Peripheral::transmit(u8 data) -> void {
  if(io.slotNumber == 0) {
    if(!memoryCardPort1.acknowledge()) {
      io.receiveSize = 1;
      io.receiveData = controllerPort1.bus(data);
      io.counter = 600;
    }

    if(!controllerPort1.acknowledge()) {
      io.receiveSize = 1;
      io.receiveData = memoryCardPort1.bus(data);
      io.counter = 600;
    }
  }

  if(io.slotNumber == 1) {
    if(!memoryCardPort2.acknowledge()) {
      io.receiveSize = 1;
      io.receiveData = controllerPort2.bus(data);
      io.counter = 600;
    }

    if(!controllerPort2.acknowledge()) {
      io.receiveSize = 1;
      io.receiveData = memoryCardPort2.bus(data);
      io.counter = 600;
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
  }

  //JOY_STAT
  if(address == 0x1f80'1044) {
    data.bit(0) = io.transmitStarted;
    data.bit(1) = io.receiveSize > 0;
    data.bit(2) = io.transmitFinished;
    data.bit(3) = io.parityError;
    data.bit(7) =!interrupt.level(Interrupt::Peripheral);
    data.bit(9) = io.interruptRequest;
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
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    data.bit(0,15) = io.baudrateReloadValue;
  }

  return data;
}

auto Peripheral::readWord(u32 address) -> u32 {
  n32 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    if(io.receiveSize) {
      data = io.receiveData;
      io.receiveData >>= 8;
      io.receiveSize--;
    }
  }

  //JOY_STAT
  if(address == 0x1f80'1044) {
    data.bit(0) = io.transmitStarted;
    data.bit(1) = io.receiveSize > 0;
    data.bit(2) = io.transmitFinished;
    data.bit(3) = io.parityError;
    data.bit(7) =!interrupt.level(Interrupt::Peripheral);
    data.bit(9) = io.interruptRequest;
  }

  return data;
}

auto Peripheral::writeByte(u32 address, u32 value) -> void {
  n8 data = value;

  //JOY_TX_DATA
  if(address == 0x1f80'1040) {
    transmit(data);
  }
}

auto Peripheral::writeHalf(u32 address, u32 value) -> void {
  n16 data = value;

  //JOY_TX_DATA
  if(address == 0x1f80'1040) {
    transmit(data);
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
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    io.baudrateReloadValue = data.bit(0,15);
  }
}

auto Peripheral::writeWord(u32 address, u32 data) -> void {
  debug(unimplemented, "Peripheral::writeWord");
}
