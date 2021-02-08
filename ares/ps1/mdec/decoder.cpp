auto MDEC::decodeMacroblocks() -> void {
  while(!fifo.input.empty()) {
    u32 output[256];

    if(status.outputDepth <= 1) {
      if(!decodeBlock(block.y0, block.luma)) break;
      convertY(output, block.y0);
    }

    if(status.outputDepth >= 2) {
      if(!decodeBlock(block.cr, block.chroma)) break;
      if(!decodeBlock(block.cb, block.chroma)) break;
      if(!decodeBlock(block.y0, block.luma)) break;
      if(!decodeBlock(block.y1, block.luma)) break;
      if(!decodeBlock(block.y2, block.luma)) break;
      if(!decodeBlock(block.y3, block.luma)) break;
      convertYUV(output, block.y0, 0, 0);
      convertYUV(output, block.y1, 8, 0);
      convertYUV(output, block.y2, 0, 8);
      convertYUV(output, block.y3, 8, 8);
    }

    //4-bit
    if(status.outputDepth == 0) {
      for(u32 index = 0; index < 64; index += 8) {
        u32 a = (output[index + 0] >> 4) <<  0;
        u32 b = (output[index + 1] >> 4) <<  4;
        u32 c = (output[index + 2] >> 4) <<  8;
        u32 d = (output[index + 3] >> 4) << 12;
        u32 e = (output[index + 4] >> 4) << 16;
        u32 f = (output[index + 5] >> 4) << 20;
        u32 g = (output[index + 6] >> 4) << 24;
        u32 h = (output[index + 7] >> 4) << 28;
        fifo.output.write(a | b | c | d | e | f | g | h);
      }
    }

    //8-bit
    if(status.outputDepth == 1) {
      for(u32 index = 0; index < 64; index += 4) {
        u32 a = output[index + 0] <<  0;
        u32 b = output[index + 1] <<  8;
        u32 c = output[index + 2] << 16;
        u32 d = output[index + 3] << 24;
        fifo.output.write(a | b | c | d);
      }
    }

    //15-bit
    if(status.outputDepth == 3) {
      for(u32 index = 0; index < 256; index += 2) {
        u32 a = GPU::Color::to16(output[index + 0]) <<  0 | status.outputMaskBit << 15;
        u32 b = GPU::Color::to16(output[index + 1]) << 16 | status.outputMaskBit << 31;
        fifo.output.write(a | b);
      }
    }

    //24-bit
    if(status.outputDepth == 2) {
      u32 index = 0;
      u32 state = 0;
      u32 rgb = 0;
      while(index < 256) {
        switch(state) {
        case 0:
          rgb = output[index++];
          break;
        case 1:
          rgb |= output[index] << 24;
          fifo.output.write(rgb);
          rgb = output[index++] >> 8;
          break;
        case 2:
          rgb |= output[index] << 16;
          fifo.output.write(rgb);
          rgb = output[index++] >> 16;
          break;
        case 3:
          rgb |= output[index++] << 8;
          fifo.output.write(rgb);
          break;
        }
        state = state + 1 & 3;
      }
    }
  }
  status.outputEmpty = fifo.output.empty();
}

auto MDEC::decodeBlock(s16 block[64], u8 table[64]) -> bool {
  for(u32 n : range(64)) block[n] = 0;

  maybe<u16> dct = fifo.input.read();
  if(!dct) return false;
  //skip block padding
  while(!fifo.input.empty() && *dct == 0xfe00) dct = fifo.input.read();
  s32 current = (i10)*dct;  //direct current
  u16 qfactor = *dct >> 10;   //quantization factor

  s32 value = current * table[0];
  for(u32 n = 0; n < 64;) {
    if(qfactor == 0) value = current << 1;
    value = sclamp<11>(value);

    if(qfactor > 0) {
      block[zagzig[n]] = value;
    } else if(qfactor == 0) {
      block[n] = value;
    }

    maybe<u16> rle = fifo.input.read();
    if(!rle) return false;
    current = (i10)*rle;
    n += (*rle >> 10) + 1;
    if(n >= 64) break;

    value = (current * table[n] * qfactor + 4) / 8;
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
        sum += source[y + z * 8] * block.scale[x + z * 8];
      }
      if constexpr(Pass == 0) target[x + y * 8] = sum + 0x8000 >> 16;
      if constexpr(Pass == 1) target[x + y * 8] = sclamp<8>(sclip<9>(sum + 0x8000 >> 16));
    }
  }
}

auto MDEC::convertY(u32 output[64], s16 luma[64]) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y = (i10)luma[x + y * 8];
      Y = uclamp<8>(Y + 128);
      output[x + y * 8] = Y;
    }
  }
}

auto MDEC::convertYUV(u32 output[256], s16 luma[64], u32 bx, u32 by) -> void {
  for(u32 y : range(8)) {
    for(u32 x : range(8)) {
      s16 Y  = luma[x + y * 8];
      s16 Cb = block.cb[(x + bx >> 1) + (y + by >> 1) * 8];
      s16 Cr = block.cr[(x + bx >> 1) + (y + by >> 1) * 8];

      s32 R = Y + (1.402 * Cr);
      s32 G = Y - (0.334 * Cb) - (0.714 * Cr);
      s32 B = Y + (1.722 * Cb);

      u8 r = uclamp<8>(R + 128);
      u8 g = uclamp<8>(G + 128);
      u8 b = uclamp<8>(B + 128);

      output[(x + bx) + (y + by) * 16] = r << 0 | g << 8 | b << 16;
    }
  }
}
