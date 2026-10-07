static auto divideByPowerOfTwoFloor(s64 value, u32 shift) -> s32 {
  u64 divisor = 1ull << shift;
  if(value >= 0) return value / divisor;
  return -s32((u64(-value) + divisor - 1) / divisor);
}

auto MDEC::advanceDecode() -> void {
  auto abort = [&] {
    io.mode = Mode::Idle;
    io.decodePhase = DecodeIdle;
    io.blockPhase = BlockStart;
    io.phaseClocks = 0;
  };
  auto selectCurrentBlock = [&] {
    switch(io.decodePhase) {
    case DecodeCr: status.currentBlock = 4; return;
    case DecodeCb: status.currentBlock = 5; return;
    case DecodeY0: status.currentBlock = status.outputDepth < 2 ? 4 : 0; return;
    case DecodeY1: status.currentBlock = 1; return;
    case DecodeY2: status.currentBlock = 2; return;
    case DecodeY3: status.currentBlock = 3; return;
    default: return;
    }
  };
  auto decode = [&](s16 target[64], u8 table[64], DecodePhase next) {
    BlockProgress progress = advanceBlock(target, table);
    if(progress == BlockAborted) return abort();
    if(progress == BlockComplete) {
      io.decodePhase = next;
      io.phaseClocks = next == Convert ? 0 : ReferenceBlockClocks;
      selectCurrentBlock();
    }
  };

  switch(io.decodePhase) {
  case DecodeIdle:
    return abort();
  case DecodeCr:
    status.currentBlock = 4;
    return decode(block.cr, block.chroma, DecodeCb);
  case DecodeCb:
    status.currentBlock = 5;
    return decode(block.cb, block.chroma, DecodeY0);
  case DecodeY0:
    status.currentBlock = status.outputDepth < 2 ? 4 : 0;
    return decode(block.y0, block.luma, status.outputDepth < 2 ? Convert : DecodeY1);
  case DecodeY1:
    status.currentBlock = 1;
    return decode(block.y1, block.luma, DecodeY2);
  case DecodeY2:
    status.currentBlock = 2;
    return decode(block.y2, block.luma, DecodeY3);
  case DecodeY3:
    status.currentBlock = 3;
    return decode(block.y3, block.luma, Convert);
  case Convert:
    if(status.outputDepth < 2) {
      convertY(output, block.y0);
    } else {
      convertYUV(output, block.y0, 0, 0);
      convertYUV(output, block.y1, 8, 0);
      convertYUV(output, block.y2, 0, 8);
      convertYUV(output, block.y3, 8, 8);
    }
    io.outputOffset = 0;
    io.outputBlock = 0;
    io.outputWriteOffset = 0;
    io.blockPhase = BlockStart;
    io.decodePhase = Publish;
    return;
  case Publish:
    status.currentBlock = status.outputDepth < 2 ? 4 : 0;
    fillOutputFifo();
    io.decodePhase = Drain;
    return;
  case Drain:
    fillOutputFifo();
    if(io.outputBlock < (status.outputDepth < 2 ? 1 : 4) || !fifo.output.empty()) return;
    if(status.remaining.empty()) return abort();
    io.outputOffset = 0;
    io.outputBlock = 0;
    io.outputWriteOffset = 0;
    io.blockPhase = BlockStart;
    io.coefficient = 0;
    io.qfactor = 0;
    io.phaseClocks = referenceDecodeClocks();
    io.decodePhase = status.outputDepth < 2 ? DecodeY0 : DecodeCr;
    selectCurrentBlock();
    return;
  }
}

auto MDEC::outputWordsPerBlock() const -> u32 {
  static constexpr u32 words[4] = {8, 16, 48, 32};
  return words[status.outputDepth];
}

auto MDEC::referenceDecodeClocks() const -> u32 {
  return status.outputDepth < 2 ? ReferenceMacroblockClocks : ReferenceBlockClocks;
}

auto MDEC::outputPixel(u32 index) const -> u32 {
  if(status.outputDepth < 2) return output[index];
  u32 outputBlock = index >> 6;
  u32 blockPixel = index & 63;
  u32 x = (outputBlock & 1) * 8 + (blockPixel & 7);
  u32 y = (outputBlock >> 1) * 8 + (blockPixel >> 3);
  return output[x + y * 16];
}

auto MDEC::outputWord(u32 outputBlock, u32 word) const -> u32 {
  u32 firstPixel = status.outputDepth < 2 ? 0 : outputBlock * 64;
  if(status.outputDepth == 0) {
    u32 index = firstPixel + word * 8;
    u32 a = (outputPixel(index + 0) >> 4) <<  0;
    u32 b = (outputPixel(index + 1) >> 4) <<  4;
    u32 c = (outputPixel(index + 2) >> 4) <<  8;
    u32 d = (outputPixel(index + 3) >> 4) << 12;
    u32 e = (outputPixel(index + 4) >> 4) << 16;
    u32 f = (outputPixel(index + 5) >> 4) << 20;
    u32 g = (outputPixel(index + 6) >> 4) << 24;
    u32 h = (outputPixel(index + 7) >> 4) << 28;
    return a | b | c | d | e | f | g | h;
  }

  if(status.outputDepth == 1) {
    u32 index = firstPixel + word * 4;
    return outputPixel(index + 0) << 0 | outputPixel(index + 1) << 8
      | outputPixel(index + 2) << 16 | outputPixel(index + 3) << 24;
  }

  if(status.outputDepth == 3) {
    u32 index = firstPixel + word * 2;
    u32 a = output15(outputPixel(index + 0)) | status.outputMaskBit << 15;
    u32 b = output15(outputPixel(index + 1)) << 16 | status.outputMaskBit << 31;
    return a | b;
  }

  u32 result = 0;
  for(u32 byte : range(4)) {
    u32 blockByte = word * 4 + byte;
    u32 pixel = firstPixel + blockByte / 3;
    result |= ((outputPixel(pixel) >> (blockByte % 3) * 8) & 0xff) << byte * 8;
  }
  return result;
}

auto MDEC::output15(u32 color) const -> u16 {
  auto reduce = [](u32 channel) -> u32 { return min((channel + 4) >> 3, 31u); };
  return reduce(color >> 0 & 0xff) << 0
    | reduce(color >> 8 & 0xff) << 5
    | reduce(color >> 16 & 0xff) << 10;
}

auto MDEC::fillOutputFifo() -> void {
  u32 blocks = status.outputDepth < 2 ? 1 : 4;
  u32 words = outputWordsPerBlock();
  while(!fifo.output.full() && io.outputBlock < blocks) {
    writeOutputFifo(outputWord(io.outputBlock, io.outputWriteOffset++));
    if(io.outputWriteOffset == words) {
      io.outputWriteOffset = 0;
      io.outputBlock++;
    }
  }
}


auto MDEC::advanceBlock(s16 target[64], u8 table[64]) -> BlockProgress {
  if(io.blockPhase == BlockStart) {
    while(true) {
      if(status.remaining.empty()) return BlockAborted;
      if(fifo.input.empty()) return BlockWaiting;
      u16 dct = *readInputFifo();
      if(dct == 0xfe00) continue;

      for(u32 index : range(64)) target[index] = 0;
      io.coefficient = 0;
      io.qfactor = dct >> 10;
      s32 current = (i10)dct;
      s32 value = io.qfactor == 0 ? current * 32
        : current * table[0] * 16 + (current < 0 ? 8 : current > 0 ? -8 : 0);
      value = max(-0x4000, min(value, 0x3fff));
      target[0] = value;
      io.blockPhase = BlockRLE;
      break;
    }
  }

  while(io.blockPhase == BlockRLE) {
    if(status.remaining.empty()) return BlockAborted;
    if(fifo.input.empty()) return BlockWaiting;
    u16 rle = *readInputFifo();
    u32 coefficient = io.coefficient + (rle >> 10) + 1;
    if(coefficient >= 64) break;

    io.coefficient = coefficient;
    s32 current = (i10)rle;
    s32 scaledQuant = (s32)io.qfactor * table[coefficient];
    s32 value = scaledQuant == 0 ? current * 32
      : divideByPowerOfTwoFloor(current * scaledQuant, 3) * 16
        + (current < 0 ? 8 : current > 0 ? -8 : 0);
    value = max(-0x4000, min(value, 0x3fff));
    u32 raster = zagzig[coefficient];
    target[(raster & 7) * 8 + (raster >> 3)] = value;
    if(io.coefficient == 63) break;
  }

  s16 intermediate[64];
  decodeIDCT<0>(target, intermediate);
  decodeIDCT<1>(intermediate, target);
  io.blockPhase = BlockStart;
  io.coefficient = 0;
  io.qfactor = 0;
  return BlockComplete;
}

template<u32 Pass>
auto MDEC::decodeIDCT(s16 source[64], s16 target[64]) -> void {
  for(u32 x : range(8)) {
    for(u32 y : range(8)) {
      s64 sum = 0;
      for(u32 z : range(8)) {
        //The uploaded scale table remains in command order. DuckStation's new
        //path transposes it on upload, hence scale[z * 8 + y] here.
        sum += (s32)source[x * 8 + z] * block.scale[z * 8 + y];
      }
      s32 value = divideByPowerOfTwoFloor(sum + 0x20000, 18);
      if constexpr(Pass == 0) target[y * 8 + x] = value;
      if constexpr(Pass == 1) target[x * 8 + y] = sclamp<8>(sclip<9>(value));
    }
  }
}

static auto encodeMDECChannel(s32 value, bool signedOutput) -> u8 {
  s32 channel = sclamp<8>(value);
  if(signedOutput) return channel;
  return channel + 128;
}

static auto floorToMultiple(s32 value, u32 multiple) -> s32 {
  if(value >= 0) return value / s32(multiple) * s32(multiple);
  return -s32((u32(-value) + multiple - 1) / multiple * multiple);
}

auto MDEC::convertY(u32 output[64], s16 luma[64]) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y = (i10)luma[x + y * 8];
      output[x + y * 8] = encodeMDECChannel(Y, status.outputSigned);
    }
  }
}

auto MDEC::convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y  = luma[x + y * 8];
      s16 Cb = block.cb[(x + bx >> 1) + (y + by >> 1) * 8];
      s16 Cr = block.cr[(x + bx >> 1) + (y + by >> 1) * 8];

      //Reference-derived fixed-point model from DuckStation b0f7c5c1624d.
      s32 R = Y + divideByPowerOfTwoFloor(359 * Cr + 0x80, 8);
      s32 G = Y + divideByPowerOfTwoFloor(
        floorToMultiple(-88 * Cb, 32) + floorToMultiple(-183 * Cr, 8) + 0x80, 8
      );
      s32 B = Y + divideByPowerOfTwoFloor(454 * Cb + 0x80, 8);

      u8 r = encodeMDECChannel(sclip<9>(R), status.outputSigned);
      u8 g = encodeMDECChannel(sclip<9>(G), status.outputSigned);
      u8 b = encodeMDECChannel(sclip<9>(B), status.outputSigned);

      output[(x + bx) + (y + by) * 16] = r << 0 | g << 8 | b << 16;
    }
  }
}
