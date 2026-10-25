auto MDEC::canReadDMA() -> bool {
  return status.outputRequest && !fifo.output.empty();
}

auto MDEC::canWriteDMA() -> bool {
  return status.inputRequest && fifo.input.empty();
}

auto MDEC::readDMA() -> u32 {
  return readWord(0x1f80'1820);
}

auto MDEC::dmaAddress(u32 address) const -> u32 {
  if(status.outputDepth < 2) return address;
  u32 rowWords = status.outputDepth == 3 ? 4 : 6;
  u32 blockWords = rowWords * 8;
  u32 word = io.outputWord;
  u32 block = word / blockWords;
  u32 within = word % blockWords;
  u32 destination = (block / 2) * blockWords * 2
                  + (within / rowWords) * rowWords * 2
                  + (block % 2) * rowWords + within % rowWords;
  return address + (destination - word) * 4;
}

auto MDEC::writeDMA(u32 data) -> void {
  return writeWord(0x1f80'1820, data);
}

auto MDEC::readByte(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "MDEC::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto MDEC::readHalf(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "MDEC::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto MDEC::readWord(u32 address) -> u32 {
  n32 data;

  if(address == 0x1f80'1820) {
    if(fifo.output.empty()) {
      debug(unusual, "MDEC: fifo read while empty!");
    }
    bool available = !fifo.output.empty();
    data = fifo.output.read(data);
    if(available && status.outputDepth >= 2) io.outputWord = (io.outputWord + 1) % (status.outputDepth == 3 ? 128 : 192);
    if(fifo.output.empty()) processInput();
    return data;
  }

  if(address == 0x1f80'1824) {
    data.bit( 0,15) = (status.remaining / 2) - 1;
    data.bit(16,18) = status.currentBlock;
    if(!fifo.output.empty() && status.outputDepth >= 2) {
      data.bit(16,18) = io.outputWord / (status.outputDepth == 3 ? 32 : 48);
    }
    data.bit(19,22) = 0;  //unused
    data.bit(23)     = status.outputMaskBit;
    data.bit(24)     = status.outputSigned;
    data.bit(25,26) = status.outputDepth;
    data.bit(27)     = canReadDMA();
    data.bit(28)     = canWriteDMA();
    data.bit(29)     = io.mode != Mode::Idle;
    data.bit(30)     = fifo.input.size() >= 64;
    data.bit(31)     = fifo.output.empty();
    return data;
  }

  debug(unhandled, "MDEC::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto MDEC::writeByte(u32 address, u32 data) -> void {
  debug(unhandled, "MDEC::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto MDEC::writeHalf(u32 address, u32 data) -> void {
  debug(unhandled, "MDEC::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto MDEC::processInput() -> void {
  while(io.mode != Mode::DecodeMacroblock && fifo.input.size() >= 2) {
    if(io.mode == Mode::Idle && !fifo.output.empty()) break;
    n32 data = fifo.input.read(0);
    data |= u32(fifo.input.read(0)) << 16;
    if(io.mode == Mode::Idle) {
      switch(data >> 29) {
      case 1:
        io.mode = Mode::DecodeMacroblock;
        io.offset = 0;
        io.outputWord = 0;
        status.remaining = data.bit(0,15) * 2;
        if(status.remaining == 0) {
          io.mode = Mode::Idle;  //treat as 0; though it may be 65536
          debug(unhandled, "MDEC macroblock length=0");
        }
        break;
      case 2:
        io.mode = Mode::SetQuantTable;
        io.offset = 0;
        status.remaining = (data.bit(0) ? 128 : 64) / 2;
        break;
      case 3:
        io.mode = Mode::SetScaleTable;
        io.offset = 0;
        status.remaining = 64;
        break;
      }

      if(io.mode != Mode::Idle) {
        //output bits are set in status register regardless of mode
        status.outputMaskBit = data.bit(25);
        status.outputSigned  = data.bit(26);
        status.outputDepth   = data.bit(27,28);
        fifo.output.flush();
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

      status.remaining -= 2;
      if(!status.remaining) {
        io.mode = Mode::Idle;
      }
    } else if(io.mode == Mode::SetScaleTable) {
      block.scale[io.offset++ & 63] = data >>  0;
      block.scale[io.offset++ & 63] = data >> 16;
      status.remaining -= 2;
      if(!status.remaining) {
        io.mode = Mode::Idle;
      }
    }
  }
}

auto MDEC::writeWord(u32 address, u32 value) -> void {
  n32 data = value;

  if(address == 0x1f80'1820) {
    if(fifo.input.capacity() - fifo.input.size() < 2) return;
    fifo.input.write(data >> 0);
    fifo.input.write(data >> 16);
    processInput();
    return;
  }

  if(address == 0x1f80'1824) {
    if(data.bit(31)) {  //reset
      io.mode = Mode::Idle;
      io.offset = 0;
      io.outputWord = 0;
      fifo.input.flush();
      fifo.output.flush();

      status.remaining     = 0;
      status.currentBlock  = 4;
      status.outputMaskBit = 0;
      status.outputSigned  = 0;
      status.outputDepth   = 0;
      status.outputRequest = 0;
      status.inputRequest  = 0;
    }

    status.outputRequest = data.bit(29);
    status.inputRequest  = data.bit(30);
    return;
  }

  debug(unhandled, "MDEC::writeWord(", hex(address, 8L), ", ", hex(value, 8L), ")");
}

auto MDEC::readInputFifo() -> maybe<u16> {
  if(status.remaining == 0) {
    return nothing;
  }

  while(fifo.input.empty()) {
    step(128);
    if(status.remaining == 0) return nothing;
  }

  step(1);
  if(status.remaining == 0 || fifo.input.empty()) return nothing;
  auto data = *fifo.input.read();
  --status.remaining;
  return data;
}


auto MDEC::writeOutputFifo(u32 data) -> void {
  while(!fifo.output.write(data)) step(128);
}

