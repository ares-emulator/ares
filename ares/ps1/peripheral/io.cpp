auto Peripheral::readByte(u32 address) -> u32 {
  uint8 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    if(io.receiveSize) {
      data = io.receiveData;
      io.receiveData >>= 8;
      io.receiveSize--;
    }
  }

//print("* rb", hex(address, 8L), " = ", hex(data, 2L), "\n");
  return data;
}

auto Peripheral::readHalf(u32 address) -> u32 {
  uint16 data;

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
    data.bit(7) = io.acknowledgeLine == 0;
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

//print("* rh", hex(address, 8L), " = ", hex(data, 4L), "\n");
  return data;
}

auto Peripheral::readWord(u32 address) -> u32 {
  uint32 data;

  //JOY_RX_DATA
  if(address == 0x1f80'1040) {
    if(io.receiveSize) {
      data = io.receiveData;
      io.receiveData >>= 8;
      io.receiveSize--;
    }
  }

  print("* rw", hex(address, 8L), "\n");
  return data;
}

auto Peripheral::writeByte(u32 address, u32 value) -> void {
  uint8 data = value;

  //JOY_TX_DATA
  if(address == 0x1f80'1040) {
    io.transmitData = data;
    io.receiveEnable = 1;
  }

//print("* wb", hex(address, 8L), " = ", hex(data, 2L), "\n");
}

auto Peripheral::writeHalf(u32 address, u32 value) -> void {
  uint16 data = value;

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

    if(io.acknowledge || io.reset) {
      io.acknowledgeLine = 0;
      interrupt.lower(Interrupt::Peripheral);
    }

    if(io.transmitEnable) {
      io.transmitStarted = 1;
      if(io.transmitData == 0x01) {
        io.mode = IO::Mode::ControllerAccess;
        io.counter = 340;
      } else if(io.transmitData == 0x42) {
        io.mode = IO::Mode::ControllerIDLower;
        io.counter = 340;
      } else if(io.transmitData == 0x00 && io.mode == IO::Mode::ControllerIDLower) {
        io.mode = IO::Mode::ControllerIDUpper;
        io.counter = 340;
      } else if(io.transmitData == 0x00 && io.mode == IO::Mode::ControllerIDUpper) {
        io.mode = IO::Mode::ControllerDataLower;
        io.counter = 340;
      } else if(io.transmitData == 0x00 && io.mode == IO::Mode::ControllerDataLower) {
        io.mode = IO::Mode::ControllerDataUpper;
        io.counter = 340;
      }
    }
  }

  //JOY_BAUD
  if(address == 0x1f80'104e) {
    io.baudrateReloadValue = data.bit(0,15);
  }

//print("* wh", hex(address, 8L), " = ", hex(data, 4L), "\n");
}

auto Peripheral::writeWord(u32 address, u32 data) -> void {
  print("* ww", hex(address, 8L), " = ", hex(data, 8L), "\n");
}
