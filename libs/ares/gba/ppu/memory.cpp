auto PPU::releaseBus() -> void {
  pramAccessed = false;
  vramAccessedBG = false;
}

auto PPU::pramContention() -> bool {
  if(accurate) return pramAccessed;

  //approximate timings in scanline renderer, assuming no layer blending
  n3 mode = PPU::Background::IO::mode;
  if(display.io.vcounter < 160 && !ppu.blank() && mode != 3 && mode != 5) {
    n32 hcounter = cpu.context.hcounter;
    if(hcounter < 46 || hcounter > 1005) return false;
    return (hcounter & 3) == 2;
  }
  return false;
}

auto PPU::vramContention(n32 address) -> bool {
  address &= 0x1ffff;
  if(Background::IO::mode < 3 && address >= 0x10000) return false;
  else if(address >= 0x14000) return false;
  return vramAccessedBG;
}

auto PPU::readVRAM_BG(u32 mode, n32 address) -> n16 {
  if(Background::IO::mode < 3 && address >= 0x10000) return 0;
  else if(address >= 0x14000) return 0;
  vramAccessedBG = true;

  return readVRAM(mode, address);
}

auto PPU::readVRAM(u32 mode, n32 address) -> n16 {
  address &= 0x1ffff;
  if(Background::IO::mode >= 3 && address < 0x1c000 && address >= 0x18000) return 0;

  address &= (address & 0x10000) ? 0x17fff : 0x0ffff;

  if(mode & Half) {
    address &= ~1;
    return vram[address + 0] << 0 | vram[address + 1] << 8;
  } else if(mode & Byte) {
    return vram[address];
  }

  unreachable;
}

auto PPU::writeVRAM(u32 mode, n32 address, n16 half) -> void {
  address &= 0x1ffff;
  if(Background::IO::mode >= 3 && address < 0x1c000 && address >= 0x18000) return;

  address &= (address & 0x10000) ? 0x17fff : 0x0ffff;

  if(mode & Half) {
    address &= ~1;
    vram[address + 0] = half >>  0;
    vram[address + 1] = half >>  8;
  } else if(mode & Byte) {
    //8-bit writes to OBJ section of VRAM are ignored
    if(Background::IO::mode <= 2 && address >= 0x10000) return;
    if(Background::IO::mode <= 5 && address >= 0x14000) return;

    address &= ~1;
    vram[address + 0] = (n8)half;
    vram[address + 1] = (n8)half;
  }
}

auto PPU::readPRAM(u32 mode, n32 address) -> n16 {
  if(mode & Byte) return readPRAM(Half, address) >> ((address & 1) * 8);
  return pram[address >> 1 & 511];
}

auto PPU::writePRAM(u32 mode, n32 address, n16 half) -> void {
  if(mode & Byte) {
    half = (n8)half;
    return writePRAM(Half, address, half << 8 | half << 0);
  }

  pram[address >> 1 & 511] = (n16)half;
}

auto PPU::readOAM(u32 mode, n32 address) -> n32 {
  if(mode & Word) return readOAM(Half, address & ~2) << 0 | readOAM(Half, address | 2) << 16;
  if(mode & Byte) return readOAM(Half, address) >> ((address & 1) * 8);

  auto& obj = object[address >> 3 & 127];
  auto& par = objectParam[address >> 5 & 31];

  switch(address & 6) {

  case 0: return (
    (obj.y          <<  0)
  | (obj.affine     <<  8)
  | (obj.affineSize <<  9)
  | (obj.mode       << 10)
  | (obj.mosaic     << 12)
  | (obj.colors     << 13)
  | (obj.shape      << 14)
  );

  case 2: return (
    (obj.x           <<  0)
  | (obj.affineParam <<  9)
  | (obj.hflip       << 12)
  | (obj.vflip       << 13)
  | (obj.size        << 14)
  );

  case 4: return (
    (obj.character <<  0)
  | (obj.priority  << 10)
  | (obj.palette   << 12)
  );

  case 6:
    switch(address >> 3 & 3) {
    case 0: return par.pa;
    case 1: return par.pb;
    case 2: return par.pc;
    case 3: return par.pd;
    }

  }

  unreachable;
}

auto PPU::writeOAM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writeOAM(Half, address & ~2, word >>  0);
    writeOAM(Half, address |  2, word >> 16);
    return;
  }

  if(mode & Byte) return;  //8-bit writes to OAM are ignored

  auto& obj = object[address >> 3 & 127];
  auto& par = objectParam[address >> 5 & 31];
  switch(address & 6) {

  case 0:
    obj.y          = word >>  0;
    obj.affine     = word >>  8;
    obj.affineSize = word >>  9;
    obj.mode       = word >> 10;
    obj.mosaic     = word >> 12;
    obj.colors     = word >> 13;
    obj.shape      = word >> 14;
    break;

  case 2:
    obj.x           = word >>  0;
    obj.affineParam = word >>  9;
    obj.hflip       = word >> 12;
    obj.vflip       = word >> 13;
    obj.size        = word >> 14;
    break;

  case 4:
    obj.character = word >>  0;
    obj.priority  = word >> 10;
    obj.palette   = word >> 12;
    break;

  case 6:
    switch(address >> 3 & 3) {
    case 0: par.pa = word; break;
    case 1: par.pb = word; break;
    case 2: par.pc = word; break;
    case 3: par.pd = word; break;
    }

  }

  static u32 widths[] = {
     8, 16, 32, 64,
    16, 32, 32, 64,
     8,  8, 16, 32,
     8,  8,  8,  8,  //invalid modes
  };

  static u32 heights[] = {
     8, 16, 32, 64,
     8,  8, 16, 32,
    16, 32, 32, 64,
     8,  8,  8,  8,  //invalid modes
  };

  obj.width  = widths [obj.shape * 4 + obj.size];
  obj.height = heights[obj.shape * 4 + obj.size];
}

auto PPU::readObjectVRAM(u32 address) const -> n8 {
  if(Background::IO::mode == 3 || Background::IO::mode == 4 || Background::IO::mode == 5) {
    if(address <= 0x3fff) return 0u;
  }
  return vram[0x10000 + (address & 0x7fff)];
}
