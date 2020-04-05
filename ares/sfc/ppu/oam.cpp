auto PPU::OAM::read(uint10 address) -> uint8 {
  uint8 data;

  if(!address.bit(9)) {
    uint7 n = address >> 2;  //object#
    address &= 3;
    if(address == 0) return object[n].x.bit(0,7);
    if(address == 1) return object[n].y;
    if(address == 2) return object[n].character;
    data.bit(0)   = object[n].nameselect;
    data.bit(1,3) = object[n].palette;
    data.bit(4,5) = object[n].priority;
    data.bit(6)   = object[n].hflip;
    data.bit(7)   = object[n].vflip;
    return data;
  }

  uint7 n = (uint5)address << 2;  //object#
  data.bit(0) = object[n + 0].x.bit(8);
  data.bit(1) = object[n + 0].size;
  data.bit(2) = object[n + 1].x.bit(8);
  data.bit(3) = object[n + 1].size;
  data.bit(4) = object[n + 2].x.bit(8);
  data.bit(5) = object[n + 2].size;
  data.bit(6) = object[n + 3].x.bit(8);
  data.bit(7) = object[n + 3].size;
  return data;
}

auto PPU::OAM::write(uint10 address, uint8 data) -> void {
  if(!address.bit(9)) {
    uint7 n = address >> 2;  //object#
    address &= 3;
    if(address == 0) { object[n].x.bit(0,7) = data; return; }
    if(address == 1) { object[n].y = data; return; }
    if(address == 2) { object[n].character = data; return; }
    object[n].nameselect = data.bit(0);
    object[n].palette    = data.bit(1,3);
    object[n].priority   = data.bit(4,5);
    object[n].hflip      = data.bit(6);
    object[n].vflip      = data.bit(7);
  } else {
    uint7 n = (uint5)address << 2;  //object#
    object[n + 0].x.bit(8) = data.bit(0);
    object[n + 0].size     = data.bit(1);
    object[n + 1].x.bit(8) = data.bit(2);
    object[n + 1].size     = data.bit(3);
    object[n + 2].x.bit(8) = data.bit(4);
    object[n + 2].size     = data.bit(5);
    object[n + 3].x.bit(8) = data.bit(6);
    object[n + 3].size     = data.bit(7);
  }
}

alwaysinline auto PPU::OAM::Object::width() const -> uint {
  if(size == 0) {
    static const uint width[] = { 8,  8,  8, 16, 16, 32, 16, 16};
    return width[ppu.obj.io.baseSize];
  } else {
    static const uint width[] = {16, 32, 64, 32, 64, 64, 32, 32};
    return width[ppu.obj.io.baseSize];
  }
}

alwaysinline auto PPU::OAM::Object::height() const -> uint {
  if(size == 0) {
    if(ppu.obj.io.interlace && ppu.obj.io.baseSize >= 6) return 16;  //hardware quirk
    static const uint height[] = { 8,  8,  8, 16, 16, 32, 32, 32};
    return height[ppu.obj.io.baseSize];
  } else {
    static const uint height[] = {16, 32, 64, 32, 64, 64, 64, 32};
    return height[ppu.obj.io.baseSize];
  }
}
