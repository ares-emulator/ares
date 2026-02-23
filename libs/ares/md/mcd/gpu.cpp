//Graphics Processing Unit

auto MCD::GPU::step(u32 clocks) -> void {
  if(!active) return;

  counter += clocks;
  while(counter >= period) {
    counter -= period;
    render(image.address, image.hdots);
    image.address += 8;
    if(!--image.vdots) {
      active = 0;
      irq.raise();
      break;
    }
  }
}

auto MCD::GPU::read(n19 address) -> n4 {
  u32 lo = 12 - ((address & 3) << 2), hi = lo + 3;
  return mcd.wram[address >> 2].bit(lo, hi);
}

auto MCD::GPU::write(n19 address, n4 data) -> void {
  u32 lo = 12 - ((address & 3) << 2), hi = lo + 3;
  mcd.wram[address >> 2].bit(lo, hi) = data;
}

auto MCD::GPU::render(n19 address, n9 width) -> void {
  n4  stampShift =  4;
  n4  mapShift   =  4 << stamp.map.size;
  n11 indexMask  = ~0;
  n5  pixelOffsetMask = 0x0f;

  if(stamp.tile.size) {
    stampShift++;
    mapShift--;
    indexMask &= ~3;
    pixelOffsetMask |= 0x10;
  }

  n19 imageWidth = image.vcells+1 << 6;
  n24 mapMask = !stamp.map.size ? 0x07ffff : 0x7fffff;

  n24 x = mcd.wram[vector.address++] << 8;  //13.3 -> 13.11
  n24 y = mcd.wram[vector.address++] << 8;  //13.3 -> 13.11

  i16 xstep = mcd.wram[vector.address++];
  i16 ystep = mcd.wram[vector.address++];

  while(width--) {
    if(stamp.repeat) {
      x &= mapMask;
      y &= mapMask;
    }

    n4 output = 0;
    if(bool outside = (x | y) & ~mapMask; !outside) {
      auto xtrunc = x >> 11;
      auto ytrunc = y >> 11;
      auto xstamp = xtrunc >> stampShift;
      auto ystamp = ytrunc >> stampShift;

      auto mapEntry = mcd.wram[stamp.map.address + (ystamp << mapShift) + xstamp];
      n11 index = mapEntry & indexMask;
      n1  lroll = mapEntry >> 13;  //0 = 0 degrees; 1 =  90 degrees
      n1  hroll = mapEntry >> 14;  //0 = 0 degrees; 1 = 180 degrees
      n1  hflip = mapEntry >> 15;

      if(index) {
        if(hflip) { xtrunc = ~xtrunc; }
        if(hroll) { xtrunc = ~xtrunc; ytrunc = ~ytrunc; }
        if(lroll) { auto t = xtrunc; xtrunc = ~ytrunc; ytrunc = t; }

        n5 xpixel = xtrunc & pixelOffsetMask;
        n5 ypixel = ytrunc & pixelOffsetMask;

        output = read(
          index  << 8 |
         (xpixel & ~7) << stampShift |
          ypixel << 3 |
          xpixel &  7);
      }
    }

    n4 input = read(address);
    switch(mcd.io.wramPriority) {
    case 0: output = output; break;
    case 1: output = input ? input : output; break;
    case 2: output = output ? output : input; break;
    case 3: output = input; break;
    }
    write(address, output);
    if(!(++address & 7)) address += imageWidth - 8;

    x += xstep;
    y += ystep;
  }
}

auto MCD::GPU::start() -> void {
  if(mcd.io.wramMode) return;  //must be in 2mbit WRAM mode

  active = 1;
  period = 5 * image.hdots;
  counter = 0;

  image.address = (image.base << 1) + image.offset;
  vector.address = vector.base >> 1;
  stamp.map.address = stamp.map.base >> 1;
  if(stamp.map.size == 0 && stamp.tile.size == 0) stamp.map.address &= 0x1ff00;  // A9-A17
  if(stamp.map.size == 0 && stamp.tile.size == 1) stamp.map.address &= 0x1ffc0;  // A5-A17
  if(stamp.map.size == 1 && stamp.tile.size == 0) stamp.map.address &= 0x10000;  //    A17
  if(stamp.map.size == 1 && stamp.tile.size == 1) stamp.map.address &= 0x1c000;  //A15-A17
}

auto MCD::GPU::power(bool reset) -> void {
  irq = {};
  font = {};
  stamp = {};
  image = {};
  vector = {};
  active = 0;
  counter = 0;
  period = 0;
}
