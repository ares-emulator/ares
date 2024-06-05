auto PPU::readCIRAM(n11 address) -> n8 {
  return ciram[address];
}

auto PPU::writeCIRAM(n11 address, n8 data) -> void {
  ciram[address] = data;
}

auto PPU::readCGRAM(n5 address) -> n8 {
  if((address & 0x3) == 0x0) address &= ~0x10;
  n8 data = cgram[address];
  if(io.grayscale) data &= 0x30;
  return data;
}

auto PPU::writeCGRAM(n5 address, n8 data) -> void {
  cgram[address] = data;

  if((address & 0x3) == 0x0)
    cgram[address ^ 0x10] = data;
}

auto PPU::readIO(n16 address) -> n8 {
  n8 result = io.mdr;

  switch(address.bit(0,2)) {

  //PPUSTATUS
  case 2:
    result.bit(5) = sprite.io.spriteOverflow;
    result.bit(6) = io.spriteZeroHit;
    result.bit(7) = io.nmiFlag;
    io.v.latch = 0;
    io.nmiHold = 0;
    cpu.nmiLine(io.nmiFlag = 0);
    break;

  //OAMDATA
  case 4:
    result = sprite.oamData();
    break;

  //PPUDATA
  case 7:
    if(enable() && (io.ly < 240 || io.ly == vlines() - 1)) break;

    address = (n14)io.v.address;
    if(address <= 0x1fff) {
      result = io.busData;
      io.busData = cartridge.readCHR(address);
    } else if(address <= 0x3eff) {
      result = io.busData;
      io.busData = cartridge.readCHR(address);
    } else if(address <= 0x3fff) {
      result.bit(0, 5) = readCGRAM(address);
      io.busData = cartridge.readCHR(address);
    }
    io.v.address += io.vramIncrement;
    break;
  }

  return io.mdr = result;
}

auto PPU::writeIO(n16 address, n8 data) -> void {
  io.mdr = data;

  switch(address.bit(0,2)) {

  //PPUCTRL
  case 0:
    io.t.nametable   = data.bit(0,1);
    io.vramIncrement = data.bit(2) ? 32 : 1;
    io.spriteAddress = data.bit(3) ? 0x1000 : 0x0000;
    io.bgAddress     = data.bit(4) ? 0x1000 : 0x0000;
    io.spriteHeight  = data.bit(5) ? 16 : 8;
    io.masterSelect  = data.bit(6);
    io.nmiEnable     = data.bit(7);
    cpu.nmiLine(io.nmiEnable && io.nmiHold && io.nmiFlag);
    break;

  //PPUMASK
  case 1:
    io.grayscale        = data.bit(0);
    io.bgEdgeEnable     = data.bit(1);
    io.spriteEdgeEnable = data.bit(2);
    io.bgEnable         = data.bit(3);
    io.spriteEnable     = data.bit(4);
    io.emphasis         = data.bit(5,7);
    break;

  //PPUSTATUS
  case 2:
    break;

  //OAMADDR
  case 3:
    sprite.io.oamAddress = data;
    break;

  //OAMDATA
  case 4:
    sprite.oamData(data);
    break;

  //PPUSCROLL
  case 5:
    if(io.v.latch++ == 0) {
      io.v.fineX = data.bit(0,2);
      io.t.tileX = data.bit(3,7);
    } else {
      io.t.fineY = data.bit(0,2);
      io.t.tileY = data.bit(3,7);
    }
    break;

  //PPUADDR
  case 6:
    if(io.v.latch++ == 0) {
      io.t.addressHi = data.bit(0,5);
    } else {
      io.t.addressLo = data.bit(0,7);
      io.v.address = io.t.address;
    }
    break;

  //PPUDATA
  case 7:
    if(enable() && (io.ly < 240 || io.ly == vlines() - 1)) return;

    address = (n14)io.v.address;
    if(address <= 0x1fff) {
      cartridge.writeCHR(address, data);
    } else if(address <= 0x3eff) {
      cartridge.writeCHR(address, data);
    } else if(address <= 0x3fff) {
      writeCGRAM(address, data);
    }
    io.v.address += io.vramIncrement;
    break;

  }
}

auto PPU::SpriteEvaluation::oamData() -> n8 const {
  n8 data = ppu.oam[io.oamAddress];

  if (ppu.io.ly < 240 || ppu.io.ly == ppu.vlines() - 1 ||
      (Region::PAL() && ppu.io.ly >= 264 && ppu.io.ly <= ppu.vlines() - 2))
    if (ppu.enable())
      return io.oamData;

  return data;
}

auto PPU::SpriteEvaluation::oamData(n8 data) -> void {
  // Writes to OAMDATA during rendering (on the pre-render
  // line and the visible lines 0-239, provided either
  // sprite or background rendering is enabled) do not
  // modify values in OAM, but do perform a glitchy
  // increment of OAMADDR, bumping only the high 6 bits
  if (ppu.io.ly < 240 || ppu.io.ly == ppu.vlines() - 1 ||
      (Region::PAL() && ppu.io.ly >= 264 && ppu.io.ly <= ppu.vlines() - 2)) {
    if (ppu.enable()) {
      ++io.oamMainCounterIndex;
      return;
    }
  }

  // The three unimplemented bits of each sprite's byte 2
  // do not exist in the PPU and always read back as 0 on
  // PPU revisions that allow reading PPU OAM through
  // OAMDATA ($2004)
  if (io.oamMainCounterTiming == 2)
    data.bit(2,4) = 0;

  ppu.oam[io.oamAddress] = data;
  ++io.oamMainCounter;
}
