auto MDEC::decodeMacroblock() -> bool {
  if(status.outputDepth <= 1) {
    status.currentBlock = 4;
    if(!decodeBlock(block.y0, block.luma)) return false;
    convertY(output, block.y0);
  }

  if(status.outputDepth >= 2) {
    status.currentBlock = 4;
    if(!decodeBlock(block.cr, block.chroma)) return false;
    status.currentBlock = 5;
    if(!decodeBlock(block.cb, block.chroma)) return false;
    status.currentBlock = 0;
    if(!decodeBlock(block.y0, block.luma)) return false;
    status.currentBlock = 1;
    if(!decodeBlock(block.y1, block.luma)) return false;
    status.currentBlock = 2;
    if(!decodeBlock(block.y2, block.luma)) return false;
    status.currentBlock = 3;
    if(!decodeBlock(block.y3, block.luma)) return false;

    convertYUV(output, block.y0, 0, 0);
    convertYUV(output, block.y1, 8, 0);
    convertYUV(output, block.y2, 0, 8);
    convertYUV(output, block.y3, 8, 8);
  }

  //4-bit
  if(status.outputDepth == 0) {
    auto nibble = [](u32 pixel) -> u32 { return min<u32>(15, (pixel + 8) >> 4); };
    for(u32 index = 0; index < 64; index += 8) {
      u32 a = nibble(output[index + 0]) <<  0;
      u32 b = nibble(output[index + 1]) <<  4;
      u32 c = nibble(output[index + 2]) <<  8;
      u32 d = nibble(output[index + 3]) << 12;
      u32 e = nibble(output[index + 4]) << 16;
      u32 f = nibble(output[index + 5]) << 20;
      u32 g = nibble(output[index + 6]) << 24;
      u32 h = nibble(output[index + 7]) << 28;
      writeOutputFifo(a | b | c | d | e | f | g | h);
    }
  }

  //8-bit
  if(status.outputDepth == 1) {
    for(u32 index = 0; index < 64; index += 4) {
      u32 a = output[index + 0] <<  0;
      u32 b = output[index + 1] <<  8;
      u32 c = output[index + 2] << 16;
      u32 d = output[index + 3] << 24;
      writeOutputFifo(a | b | c | d);
    }
  }

  //15-bit
  if(status.outputDepth == 3) {
    auto pack = [](u32 color) -> u32 {
      u32 r = min<u32>(31, ((color >>  0 & 255) + 4) >> 3);
      u32 g = min<u32>(31, ((color >>  8 & 255) + 4) >> 3);
      u32 b = min<u32>(31, ((color >> 16 & 255) + 4) >> 3);
      return r | g << 5 | b << 10;
    };
    for(u32 index = 0; index < 256; index += 2) {
      u32 source = (index / 128) * 128 + ((index / 64) % 2) * 8
                 + ((index % 64) / 8) * 16 + index % 8;
      u32 a = pack(output[source + 0]) <<  0 | status.outputMaskBit << 15;
      u32 b = pack(output[source + 1]) << 16 | status.outputMaskBit << 31;
      writeOutputFifo(a | b);
    }
  }

  //24-bit
  if(status.outputDepth == 2) {
    u32 index = 0;
    u32 state = 0;
    u32 rgb = 0;
    auto pixel = [&](u32 index) -> u32 {
      u32 source = (index / 128) * 128 + ((index / 64) % 2) * 8
                 + ((index % 64) / 8) * 16 + index % 8;
      return output[source];
    };
    while(index < 256) {
      switch(state) {
        case 0:
          rgb = pixel(index++);
          break;
        case 1:
          rgb |= pixel(index) << 24;
          writeOutputFifo(rgb);
          rgb = pixel(index++) >> 8;
          break;
        case 2:
          rgb |= pixel(index) << 16;
          writeOutputFifo(rgb);
          rgb = pixel(index++) >> 16;
          break;
        case 3:
          rgb |= pixel(index++) << 8;
          writeOutputFifo(rgb);
          break;
      }
      state = state + 1 & 3;
    }
  }

  return true;
}


auto MDEC::decodeBlock(s16 block[64], u8 table[64]) -> bool {
  step(status.outputDepth <= 1 ? 2688 : 448);
  for(u32 n : range(64)) block[n] = 0;

  maybe<u16> dct = readInputFifo();
  if(!dct) return false;
  //skip block padding
  while(*dct == 0xfe00) {
    dct = readInputFifo();
    if(!dct) return false;
  }

  s32 current = (i10)*dct;  //direct current
  u16 qfactor = *dct >> 10;   //quantization factor

  auto quantize = [](s32 value, s32 factor, bool dc) -> s32 {
    if(!factor) return value * 32;
    s32 coefficient = (dc ? value * factor : value * factor >> 3) * 16;
    if(value) coefficient += value < 0 ? 8 : -8;
    return coefficient;
  };
  s32 value = table[0] ? quantize(current, table[0], true) : 0;
  for(u32 n = 0; n < 64;) {
    if(qfactor == 0) value = current * 32;
    value = sclamp<15>(value);

    if(qfactor > 0) {
      block[zagzig[n]] = value;
    } else if(qfactor == 0) {
      block[n] = value;
    }

    if(n == 63) break;

    maybe<u16> rle = readInputFifo();
    if(!rle) return false;
    current = (i10)*rle;
    n += (*rle >> 10) + 1;
    if(n >= 64) break;

    value = quantize(current, table[n] * qfactor, false);
  }

  s16 array[64];
  decodeIDCT<0>(block, array);
  decodeIDCT<1>(array, block);
  return true;
}

template<u32 Pass>
auto MDEC::decodeIDCT(s16 source[64], s16 target[64]) -> void {
  for(u32 x : range(8)) {
    for(u32 y : range(8)) {
      s32 sum = 0;
      for(u32 z : range(8)) {
        sum += source[y + z * 8] * (block.scale[x + z * 8] >> 3);
      }
      if constexpr(Pass == 0) target[x + y * 8] = sum + 0x4000 >> 15;
      if constexpr(Pass == 1) target[x + y * 8] = sclamp<8>(sclip<9>(sum + 0x4000 >> 15));
    }
  }
}

auto MDEC::convertY(u32 output[64], s16 luma[64]) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y = (i10)luma[x + y * 8];
      output[x + y * 8] = (u8)(sclamp<8>(Y) + (status.outputSigned ? 0 : 128));
    }
  }
}

auto MDEC::convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y  = luma[x + y * 8];
      s16 Cb = block.cb[(x + bx >> 1) + (y + by >> 1) * 8];
      s16 Cr = block.cr[(x + bx >> 1) + (y + by >> 1) * 8];

      s32 R = Y + (s32)(1.402 * Cr);
      s32 G = Y + (s32)(-0.3437 * Cb - 0.7143 * Cr);
      s32 B = Y + (s32)(1.772 * Cb);

      u8 r = sclamp<8>(R) + (status.outputSigned ? 0 : 128);
      u8 g = sclamp<8>(G) + (status.outputSigned ? 0 : 128);
      u8 b = sclamp<8>(B) + (status.outputSigned ? 0 : 128);

      output[(x + bx) + (y + by) * 16] = r << 0 | g << 8 | b << 16;
    }
  }
}
