auto MDEC::readDMA() -> u32 {
  return readWord(0x1f80'1820);
}

auto MDEC::writeDMA(u32 data) -> void {
  return writeWord(0x1f80'1820, data);
}

auto MDEC::readByte(u32 address) -> u32 {
  debug(unverified, "MDEC::readByte(", hex(address, 8L), ")");
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto MDEC::readHalf(u32 address) -> u32 {
  debug(unverified, "MDEC::readHalf(", hex(address, 8L), ")");
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto MDEC::readWord(u32 address) -> u32 {
  n32 data;

  if(address == 0x1f80'1820) {
    data = fifo.output.read(data);
    if(fifo.output.empty()) status.outputEmpty = 1;
  }

  if(address == 0x1f80'1824) {
    data.bit( 0,15) = status.remaining - 1;
    data.bit(16,18) = status.currentBlock;
    data.bit(19,22) = 0;  //unused
    data.bit(23)    = status.outputMaskBit;
    data.bit(24)    = status.outputSigned;
    data.bit(25,26) = status.outputDepth;
    data.bit(27)    = status.outputRequest;
    data.bit(28)    = status.inputRequest;
    data.bit(29)    = status.commandBusy;
    data.bit(30)    = status.inputFull;
    data.bit(31)    = status.outputEmpty;
  }

  return data;
}

auto MDEC::writeByte(u32 address, u32 data) -> void {
  debug(unverified, "MDEC::writeByte(", hex(address, 8L), ")");
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto MDEC::writeHalf(u32 address, u32 data) -> void {
  debug(unverified, "MDEC::writeHalf(", hex(address, 8L), ")");
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto MDEC::writeWord(u32 address, u32 value) -> void {
  n32 data = value;

  if(address == 0x1f80'1820) {
    if(io.mode == Mode::Idle) {
      switch(data >> 29) {
      case 1:
        io.mode = Mode::DecodeMacroblock;
        io.offset = 0;
        status.remaining = data.bit(0,15);
        if(status.remaining == 0) {
          io.mode = Mode::Idle;  //treat as 0; though it may be 65536
          debug(unhandled, "MDEC macroblock length=0");
        }
        break;
      case 2:
        io.mode = Mode::SetQuantTable;
        io.offset = 0;
        status.remaining = (data.bit(0) ? 128 : 64) / 4;
        break;
      case 3:
        io.mode = Mode::SetScaleTable;
        io.offset = 0;
        status.remaining = 64 / 2;
        break;
      }

      //if a valid command was decoded above:
      if(io.mode != Mode::Idle) {
        //output bits are set in status register regardless of mode
        status.outputMaskBit = data.bit(25);
        status.outputSigned  = data.bit(26);
        status.outputDepth   = data.bit(27,28);
        status.commandBusy   = 1;
      }
    } else if(io.mode == Mode::DecodeMacroblock) {
      fifo.input.write(data >>  0);
      fifo.input.write(data >> 16);
      if(!--status.remaining) {
        io.mode = Mode::Idle;
        status.commandBusy = 0;
        decodeMacroblocks();
      }
    } else if(io.mode == Mode::SetQuantTable) {
      if(io.offset < 64) {
        block.luma[io.offset++ & 63] = data >>  0;
        block.luma[io.offset++ & 63] = data >>  8;
        block.luma[io.offset++ & 63] = data >> 16;
        block.luma[io.offset++ & 63] = data >> 24;
      } else {
        block.chroma[io.offset++ & 63] = data >>  0;
        block.chroma[io.offset++ & 63] = data >>  8;
        block.chroma[io.offset++ & 63] = data >> 16;
        block.chroma[io.offset++ & 63] = data >> 24;
      }
      if(!--status.remaining) {
        io.mode = Mode::Idle;
        status.commandBusy = 0;
      }
    } else if(io.mode == Mode::SetScaleTable) {
      block.scale[io.offset++ & 63] = data >>  0;
      block.scale[io.offset++ & 63] = data >> 16;
      if(!--status.remaining) {
        io.mode = Mode::Idle;
        status.commandBusy = 0;
      }
    }
  }

  if(address == 0x1f80'1824) {
    if(data.bit(31)) {  //reset
      io.mode = Mode::Idle;
      io.offset = 0;

      status.remaining     = 0;
      status.currentBlock  = 4;
      status.outputMaskBit = 0;
      status.outputSigned  = 0;
      status.outputDepth   = 0;
      status.outputRequest = 0;
      status.inputRequest  = 0;
      status.commandBusy   = 0;
      status.inputFull     = 0;
      status.outputEmpty   = 1;
    }

    status.outputRequest = data.bit(29);
    status.inputRequest  = data.bit(30);
  }
}
