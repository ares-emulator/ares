//Graphics Processing Unit

auto MCD::GPU::step(uint clocks) -> void {
  if(!active) return;

  counter += clocks;
  while(counter >= period) {
    counter -= period;
    render(image.address, image.hdots);
    image.address += 8;
    if(!--image.vdots) {
      active = 0;
      irq.raise();
    }
  }
}

auto MCD::GPU::read(uint19 address) -> uint4 {
  uint lo = 12 - ((address & 3) << 2), hi = lo + 3;
  return mcd.wram[address >> 2].bit(lo, hi);
}

auto MCD::GPU::write(uint19 address, uint4 data) -> void {
  uint lo = 12 - ((address & 3) << 2), hi = lo + 3;
  mcd.wram[address >> 2].bit(lo, hi) = data;
}

auto MCD::GPU::render(uint19 address, uint9 width) -> void {
   uint8 stampShift = 11 + 4 + stamp.tile.size;
   uint8 mapShift   = (4 << stamp.map.size) - stamp.tile.size;

   uint2 stampMask  = !stamp.tile.size ? 1 : 3;
  uint23 mapMask    = !stamp.map.size ? 0x07ffff : 0x7fffff;

  uint24 x = mcd.wram[vector.address++] << 8;  //13.3 -> 13.11
  uint24 y = mcd.wram[vector.address++] << 8;  //13.3 -> 13.11

   int16 xstep = mcd.wram[vector.address++];
   int16 ystep = mcd.wram[vector.address++];

  while(width--) {
    if(stamp.repeat) {
      x &= mapMask;
      y &= mapMask;
    }

    uint4 output;
    if(bool outside = (x | y) & ~mapMask; !outside) {
      auto xstamp = x >> stampShift;
      auto ystamp = y >> stampShift << mapShift;

      auto data  = mcd.wram[stamp.map.address + xstamp + ystamp];
      auto index = uint10(data >>  0);
      auto lroll =  uint1(data >> 13);  //0 = 0 degrees; 1 =  90 degrees
      auto hroll =  uint1(data >> 14);  //0 = 0 degrees; 1 = 180 degrees
      auto hflip =  uint1(data >> 15);

      if(index) {  //stamp index 0 is not rendered
        auto xpixel = uint6(x >> 11);
        auto ypixel = uint6(y >> 11);

        if(hflip) { xpixel = ~xpixel; }
        if(hroll) { xpixel = ~xpixel; ypixel = ~ypixel; }
        if(lroll) { auto tpixel = xpixel; xpixel = ~ypixel; ypixel = tpixel; }

        uint6 pixel = uint3(xpixel) + uint3(ypixel) * 8;
        xpixel = xpixel >> 3 & stampMask;
        ypixel = ypixel >> 3 & stampMask;
        uint4 cell = ypixel + xpixel * (1 + stampMask);
        output = read(index << 8 | cell << 6 | pixel);
      }
    }

    uint4 input = read(address);
    switch(mcd.io.wramPriority) {
    case 0: output = output; break;
    case 1: output = input ? input : output; break;
    case 2: output = output ? output : input; break;
    case 3: output = input; break;
    }
    write(address, output);
    if(!(++address & 7)) address += (image.vcells + 1 << 6) - 8;

    x += xstep;
    y += ystep;
  }
}

auto MCD::GPU::start() -> void {
  if(mcd.io.wramMode) return;  //must be in 2mbit WRAM mode

  active = 1;
  period = 4 * 5 * image.hdots;
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
