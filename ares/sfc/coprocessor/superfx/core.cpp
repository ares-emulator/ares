auto SuperFX::stop() -> void {
  cpu.irq(1);
}

auto SuperFX::color(n8 source) -> n8 {
  if(regs.por.highnibble) return (regs.colr & 0xf0) | (source >> 4);
  if(regs.por.freezehigh) return (regs.colr & 0xf0) | (source & 0x0f);
  return source;
}

auto SuperFX::plot(n8 x, n8 y) -> void {
  if(!regs.por.transparent) {
    if(regs.scmr.md == 3) {
      if(regs.por.freezehigh) {
        if((regs.colr & 0x0f) == 0) return;
      } else {
        if(regs.colr == 0) return;
      }
    } else {
      if((regs.colr & 0x0f) == 0) return;
    }
  }

  n8 color = regs.colr;
  if(regs.por.dither && regs.scmr.md != 3) {
    if((x ^ y) & 1) color >>= 4;
    color &= 0x0f;
  }

  n16 offset = (y << 5) + (x >> 3);
  if(offset != pixelcache[0].offset) {
    flushPixelCache(pixelcache[1]);
    pixelcache[1] = pixelcache[0];
    pixelcache[0].bitpend = 0x00;
    pixelcache[0].offset = offset;
  }

  x = (x & 7) ^ 7;
  pixelcache[0].data[x] = color;
  pixelcache[0].bitpend |= 1 << x;
  if(pixelcache[0].bitpend == 0xff) {
    flushPixelCache(pixelcache[1]);
    pixelcache[1] = pixelcache[0];
    pixelcache[0].bitpend = 0x00;
  }
}

auto SuperFX::rpix(n8 x, n8 y) -> n8 {
  flushPixelCache(pixelcache[1]);
  flushPixelCache(pixelcache[0]);

  u32 cn;  //character number
  switch(regs.por.obj ? 3 : regs.scmr.ht) {
  case 0: cn = ((x & 0xf8) << 1) + ((y & 0xf8) >> 3); break;
  case 1: cn = ((x & 0xf8) << 1) + ((x & 0xf8) >> 1) + ((y & 0xf8) >> 3); break;
  case 2: cn = ((x & 0xf8) << 1) + ((x & 0xf8) << 0) + ((y & 0xf8) >> 3); break;
  case 3: cn = ((y & 0x80) << 2) + ((x & 0x80) << 1) + ((y & 0x78) << 1) + ((x & 0x78) >> 3); break;
  }
  u32 bpp = 2 << (regs.scmr.md - (regs.scmr.md >> 1));  // = [regs.scmr.md]{ 2, 4, 4, 8 };
  u32 addr = 0x700000 + (cn * (bpp << 3)) + (regs.scbr << 10) + ((y & 0x07) * 2);
  n8  data = 0x00;
  x = (x & 7) ^ 7;

  for(u32 n : range(bpp)) {
    u32 byte = ((n >> 1) << 4) + (n & 1);  // = [n]{ 0, 1, 16, 17, 32, 33, 48, 49 };
    step(regs.clsr ? 5 : 6);
    data |= ((read(addr + byte) >> x) & 1) << n;
  }

  return data;
}

auto SuperFX::flushPixelCache(PixelCache& cache) -> void {
  if(cache.bitpend == 0x00) return;

  n8 x = cache.offset << 3;
  n8 y = cache.offset >> 5;

  u32 cn;  //character number
  switch(regs.por.obj ? 3 : regs.scmr.ht) {
  case 0: cn = ((x & 0xf8) << 1) + ((y & 0xf8) >> 3); break;
  case 1: cn = ((x & 0xf8) << 1) + ((x & 0xf8) >> 1) + ((y & 0xf8) >> 3); break;
  case 2: cn = ((x & 0xf8) << 1) + ((x & 0xf8) << 0) + ((y & 0xf8) >> 3); break;
  case 3: cn = ((y & 0x80) << 2) + ((x & 0x80) << 1) + ((y & 0x78) << 1) + ((x & 0x78) >> 3); break;
  }
  u32 bpp = 2 << (regs.scmr.md - (regs.scmr.md >> 1));  // = [regs.scmr.md]{ 2, 4, 4, 8 };
  u32 addr = 0x700000 + (cn * (bpp << 3)) + (regs.scbr << 10) + ((y & 0x07) * 2);

  for(u32 n : range(bpp)) {
    u32 byte = ((n >> 1) << 4) + (n & 1);  // = [n]{ 0, 1, 16, 17, 32, 33, 48, 49 };
    n8  data = 0x00;
    for(u32 x : range(8)) data |= ((cache.data[x] >> n) & 1) << x;
    if(cache.bitpend != 0xff) {
      step(regs.clsr ? 5 : 6);
      data &= cache.bitpend;
      data |= read(addr + byte) & ~cache.bitpend;
    }
    step(regs.clsr ? 5 : 6);
    write(addr + byte, data);
  }

  cache.bitpend = 0x00;
}
