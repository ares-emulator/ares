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

  n16 half = readVRAM(mode, address);
  if(mode & Byte) {
    if(address & 1) half >>= 8;
    half = (n8)half;
  }
  return half;
}

auto PPU::readVRAM(u32 mode, n32 address) -> n16 {
  address &= 0x1ffff;
  if(Background::IO::mode >= 3 && address < 0x1c000 && address >= 0x18000) return 0;

  address &= (address & 0x10000) ? 0x17ffe : 0x0fffe;
  return vram[address + 0] << 0 | vram[address + 1] << 8;
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
  address = (n10)address;
  return oam[address >> 1 & ~1] << 0 | oam[address >> 1 | 1] << 16;
}

auto PPU::writeOAM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writeOAM(Half, address & ~2, word >>  0);
    writeOAM(Half, address |  2, word >> 16);
    return;
  }

  if(mode & Byte) return;  //8-bit writes to OAM are ignored
  oam[address >> 1 & 511] = word;
}

auto PPU::readObjectVRAM(u32 address) const -> n8 {
  if(Background::IO::mode == 3 || Background::IO::mode == 4 || Background::IO::mode == 5) {
    if(address <= 0x3fff) return 0u;
  }
  return vram[0x10000 + (address & 0x7fff)];
}
