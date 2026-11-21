auto MDEC::decodeMacroblock() -> bool {
  io.decodeBlock = 4;
  if(status.outputDepth <= 1) {
    if(!decodeBlock(block.y0)) return false;
    convertY(output, block.y0);
    writeBlock();
  } else {
    if(!decodeBlock(block.cr)) return false;
    io.decodeBlock = 5;
    if(!decodeBlock(block.cb)) return false;
    s16* luma[] = {block.y0, block.y1, block.y2, block.y3};
    for(u32 index : range(4)) {
      io.decodeBlock = index;
      if(!decodeBlock(luma[index])) return false;
      u32 bx = (index & 1) * 8;
      u32 by = (index >> 1) * 8;
      convertYUV(output, luma[index], bx, by);
      writeBlock(bx, by);
    }
  }
  return !io.resetPending;
}

auto MDEC::writeBlock(u32 bx, u32 by) -> void {
  //4-bit
  if(status.outputDepth == 0) {
    auto nibble = [&](u32 pixel) -> u32 {
      if(status.outputSigned) return sclamp<4>(((i8)pixel + 8) >> 4) & 15;
      return min<u32>(15, (pixel + 8) >> 4);
    };
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
    auto pack = [&](u32 color) -> u32 {
      auto channel = [&](u8 value) -> u32 {
        if(status.outputSigned) return sclamp<5>(((i8)value + 4) >> 3) & 31;
        return min<u32>(31, (value + 4) >> 3);
      };
      u32 r = channel(color >>  0);
      u32 g = channel(color >>  8);
      u32 b = channel(color >> 16);
      return r | g << 5 | b << 10;
    };
    for(u32 index = 0; index < 64; index += 2) {
      u32 source = (by + index / 8) * 16 + bx + index % 8;
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
      u32 source = (by + index / 8) * 16 + bx + index % 8;
      return output[source];
    };
    while(index < 64) {
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

}


auto MDEC::prefetchInput() -> void {
  if(io.mode != Mode::DecodeMacroblock || io.resetPending || prefetch.ready) return;
  auto quantize = [](s32 value, s32 factor, bool dc) -> s32 {
    s32 coefficient = dc ? value * factor * 2 : value * factor >> 2;
    if(coefficient && !(coefficient & 1)) coefficient += coefficient < 0 ? 1 : -1;
    return max(-2047, min(2047, coefficient)) * 8;
  };
  auto finish = [&] {
    prefetch.ready = 1;
    prefetch.started = 0;
  };
  u8* table = prefetch.block >= 4 && status.outputDepth >= 2 ? block.chroma : block.luma;
  while(auto code = readInputFifo()) {
    s32 current = (i10)*code;
    s32 value;
    if(!prefetch.started) {
      if(*code == 0xfe00) continue;
      for(auto& coefficient : prefetch.coefficient) coefficient = 0;
      prefetch.index = 0;
      prefetch.qfactor = *code >> 10;
      prefetch.started = 1;
      value = table[0] ? quantize(current, table[0], true) : 0;
    } else {
      u32 index = prefetch.index + (*code >> 10) + 1;
      if(index >= 64) { finish(); return; }
      prefetch.index = index;
      value = quantize(current, table[index] * prefetch.qfactor, false);
    }
    if(prefetch.qfactor == 0) value = current * 32;
    u32 index = prefetch.qfactor ? zagzig[prefetch.index] : (u32)prefetch.index;
    prefetch.coefficient[index] = sclamp<12>(value >> 3) * 8;
    if(prefetch.index == 63) { finish(); return; }
  }
}

auto MDEC::decodeBlock(s16 target[64]) -> bool {
  u32 halfBlock = status.outputDepth == 3 ? 16 : 24;
  while(!io.resetPending) {
    prefetchInput();
    bool available = prefetch.ready || io.remaining == 0;
    if(available && fifo.pendingOutput.size() <= halfBlock + 1) break;
    step(128);
  }
  if(io.resetPending) return false;
  status.currentBlock = io.decodeBlock;
  if(!prefetch.ready) return false;
  for(u32 index : range(64)) target[index] = prefetch.coefficient[index];
  prefetch.ready = 0;
  prefetch.block = status.outputDepth <= 1 ? 4 : (prefetch.block + 1) % 6;
  if(status.outputDepth >= 2 && io.decodeBlock == 4) {
    while(!io.resetPending && fifo.pendingOutput.size() > halfBlock / 2 + 1) step(128);
  }
  if(io.resetPending) return false;
  step(448);
  if(io.resetPending) return false;
  s16 array[64];
  decodeIDCT<0>(target, array);
  decodeIDCT<1>(array, target);
  return true;
}

template<u32 Pass>
auto MDEC::decodeIDCT(s16 source[64], s16 target[64]) -> void {
  for(u32 x : range(8)) {
    s32 sums[8] = {};
    for(u32 z : range(8)) {
      s32 scale = block.scale[x + z * 8] >> (Pass == 0 ? 3 : 4);
      for(u32 y : range(8)) {
        s32 coefficient = source[y + z * 8];
        if constexpr(Pass == 0) coefficient >>= 3;
        sums[y] += coefficient * scale >> 7;
      }
    }
    for(u32 y : range(8)) {
      s32 sum = sclip<17>(sums[y]);
      if constexpr(Pass == 0) target[x + y * 8] = sclip<13>(sum >> 4);
      if constexpr(Pass == 1) target[x + y * 8] = sclamp<8>((sclip<10>(sum >> 7) + 1) >> 1);
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

      auto component = [&](s32 value) -> u8 {
        s32 channel = status.outputDepth == 2
          ? (sclip<10>(value >> 9) + 1) >> 1
          : sclip<9>(value >> 10);
        return sclamp<8>(channel) + (status.outputSigned ? 0 : 128);
      };
      u8 r = component(Y * 1024 + 1436 * Cr);
      u8 g = component(Y * 1024 + (-352 * Cb & ~127) + (-732 * Cr & ~31));
      u8 b = component(Y * 1024 + 1816 * Cb);

      output[(x + bx) + (y + by) * 16] = r << 0 | g << 8 | b << 16;
    }
  }
}
