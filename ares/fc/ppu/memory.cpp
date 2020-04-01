auto PPU::readCIRAM(uint11 address) -> uint8 {
  return ciram[address];
}

auto PPU::writeCIRAM(uint11 address, uint8 data) -> void {
  ciram[address] = data;
}

auto PPU::readCGRAM(uint5 address) -> uint8 {
  if((address & 0x13) == 0x10) address &= ~0x10;
  uint8 data = cgram[address];
  if(io.grayscale) data &= 0x30;
  return data;
}

auto PPU::writeCGRAM(uint5 address, uint8 data) -> void {
  if((address & 0x13) == 0x10) address &= ~0x10;
  cgram[address] = data;
}

auto PPU::readIO(uint16 address) -> uint8 {
  uint8 result = 0x00;

  switch(address.bit(0,2)) {

  //PPUSTATUS
  case 2:
    result |= io.mdr.bit(0,4);
    result |= io.spriteOverflow << 5;
    result |= io.spriteZeroHit << 6;
    result |= io.nmiFlag << 7;
    io.v.latch = 0;
    io.nmiHold = 0;
    cpu.nmiLine(io.nmiFlag = 0);
    break;

  //OAMDATA
  case 4:
    result = oam[io.oamAddress];
    break;

  //PPUDATA
  case 7:
    if(enable() && (io.ly <= 240 || io.ly == 261)) return 0x00;

    address = (uint14)io.v.address;
    if(address <= 0x1fff) {
      result = io.busData;
      io.busData = cartridge.readCHR(address);
    } else if(address <= 0x3eff) {
      result = io.busData;
      io.busData = cartridge.readCHR(address);
    } else if(address <= 0x3fff) {
      result = readCGRAM(address);
      io.busData = cartridge.readCHR(address);
    }
    io.v.address += io.vramIncrement;
    break;

  }

  return result;
}

auto PPU::writeIO(uint16 address, uint8 data) -> void {
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
    io.oamAddress = data;
    break;

  //OAMDATA
  case 4:
    if(io.oamAddress.bit(0,1) == 2) data.bit(2,4) = 0;  //clear non-existent bits (always read back as 0)
    oam[io.oamAddress++] = data;
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
    if(enable() && (io.ly <= 240 || io.ly == 261)) return;

    address = (uint14)io.v.address;
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
