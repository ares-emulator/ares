auto MDEC::canReadDMA() -> bool {
  //Keep the existing 32-word ceiling until measured, but do not strand a complete shorter mono block.
  return status.outputRequest && fifo.output.size() >= min(32u, outputWordsPerBlock());
}

auto MDEC::canWriteDMA() -> bool {
  return status.inputRequest && fifo.input.empty();
}

auto MDEC::dmaOffset() const -> s32 {
  if(status.outputDepth < 2) return 0;
  u32 wordsPerRow = status.outputDepth == 2 ? 6 : 4;
  u32 line = io.outputOffset / wordsPerRow & 31;
  s32 offset = (line & 7) * wordsPerRow;
  if(line & 8) offset -= wordsPerRow * 7;
  return offset;
}

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
    bool outputAvailable = !fifo.output.empty();
    if(!outputAvailable) {
      debug(unusual, "MDEC: fifo read while empty!");
      return 0xffff'ffff;
    }
    data = fifo.output.read(data);
    if(outputAvailable) {
      io.outputOffset++;
      if(io.mode == Mode::DecodeMacroblock && io.decodePhase == DecodePhase::Drain) {
        fillOutputFifo();
      }
    }
    return data;
  }

  if(address == 0x1f80'1824) {
    data.bit( 0,15) = status.remaining.read();
    data.bit(16,18) = fifo.output.empty() || status.outputDepth < 2
      ? u32(status.currentBlock) : io.outputOffset / outputWordsPerBlock() & 3;
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
      //Output bits are reflected for every command, including no-ops.
      status.outputMaskBit = data.bit(25);
      status.outputSigned  = data.bit(26);
      status.outputDepth   = data.bit(27,28);
      fifo.output.flush();
      io.outputOffset = 0;

      switch(data >> 29) {
      case 0:
      case 4:
      case 5:
      case 6:
      case 7:
        status.remaining.setHalfwords(u32(data.bit(0,15)) * 2);
        io.mode = status.remaining.empty() ? Mode::Idle : Mode::NoCommand;
        break;
      case 1:
        io.mode = Mode::DecodeMacroblock;
        io.decodePhase = status.outputDepth < 2 ? DecodeY0 : DecodeCr;
        io.blockPhase = BlockStart;
        io.offset = 0;
        io.outputOffset = 0;
        io.outputBlock = 0;
        io.outputWriteOffset = 0;
        io.coefficient = 0;
        io.qfactor = 0;
        io.phaseClocks = referenceDecodeClocks();
        status.currentBlock = 4;
        status.remaining.setHalfwords(data.bit(0,15) * 2);
        if(status.remaining.empty()) {
          io.mode = Mode::Idle;  //treat as 0; though it may be 65536
          io.decodePhase = DecodeIdle;
          io.phaseClocks = 0;
          debug(unhandled, "MDEC macroblock length=0");
        }
        break;
      case 2:
        io.mode = Mode::SetQuantTable;
        io.offset = 0;
        status.remaining.setHalfwords((data.bit(0) ? 128 : 64) / 2);
        break;
      case 3:
        io.mode = Mode::SetScaleTable;
        io.offset = 0;
        status.remaining.setHalfwords(64);
        break;
      }

    } else if(io.mode == Mode::DecodeMacroblock) {
      fifo.input.write(data >>  0);
      fifo.input.write(data >> 16);
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

      status.remaining.consume(2);
      if(status.remaining.empty()) {
        io.mode = Mode::Idle;
      }
    } else if(io.mode == Mode::SetScaleTable) {
      block.scale[io.offset++ & 63] = data >>  0;
      block.scale[io.offset++ & 63] = data >> 16;
      status.remaining.consume(2);
      if(status.remaining.empty()) {
        io.mode = Mode::Idle;
      }
    } else if(io.mode == Mode::NoCommand) {
      status.remaining.consume(min(2u, status.remaining.halfwords()));
      if(status.remaining.empty()) io.mode = Mode::Idle;
    }

    return;
  }

  if(address == 0x1f80'1824) {
    if(data.bit(31)) {  //reset
      io.mode = Mode::Idle;
      io.decodePhase = DecodeIdle;
      io.blockPhase = BlockStart;
      io.offset = 0;
      io.outputOffset = 0;
      io.outputBlock = 0;
      io.outputWriteOffset = 0;
      io.coefficient = 0;
      io.qfactor = 0;
      io.phaseClocks = 0;

      fifo.input.flush();
      fifo.output.flush();

      status.remaining.setDirect(0);
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
  if(status.remaining.empty() || fifo.input.empty()) return nothing;
  auto data = *fifo.input.read();
  status.remaining.consume(1);
  return data;
}


auto MDEC::writeOutputFifo(u32 data) -> void {
  if(!fifo.output.write(data)) debug(unusual, "MDEC: output block staging overflow");
}

