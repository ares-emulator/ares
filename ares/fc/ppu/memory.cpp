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
    result.bit(5) = sprite.spriteOverflow;
    result.bit(6) = io.spriteZeroHit;
    result.bit(7) = io.nmiFlag;
    scroll.latch = 0;
    io.nmiHold = 0;
    cpu.nmiLine(io.nmiFlag = 0);
    break;

  //OAMDATA
  case 4:
    result = oam[sprite.oamAddress];

    if (io.ly < 240 || io.ly == vlines() - 1 ||
        (Region::PAL() && io.ly >= 264 && io.ly <= vlines() - 2))
      if (enable())
        result = sprite.oamData;
    break;

  //PPUDATA
  case 7:
    if (var.blockingRead) {
      result = io.mdr;
      break;
    }

    address = io.busAddress;
    result = var.latchData;
    var.latchData = cartridge.readCHR(address);
    if (address >= 0x3f00)
      result = readCGRAM(address) | (io.mdr & 0xc0);

    if (rendering()) {
      incrementVRAMAddressX();
      incrementVRAMAddressY();
    } else {
      var.address += io.vramIncrement;
      io.busAddress = var.address;
      cartridge.ppuAddressBus(io.busAddress);
    }
    
    var.blockingRead = 6;
    break;
  }

  return io.mdr = result;
}

auto PPU::writeIO(n16 address, n8 data) -> void {
  io.mdr = data;

  switch(address.bit(0,2)) {

  //PPUCTRL
  case 0:
    scroll.nametable = data.bit(0,1);
    if (ctrlGlitch->value() && io.lx == 257 && rendering())
      var.nametableX = cpu.io.openBus.bit(0);

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
    sprite.oamAddress = data;
    break;

  //OAMDATA
  case 4:
    // Writes to OAMDATA during rendering (on the pre-render
    // line and the visible lines 0-239, provided either
    // sprite or background rendering is enabled) do not
    // modify values in OAM, but do perform a glitchy
    // increment of OAMADDR, bumping only the high 6 bits
    if (io.ly < 240 || io.ly == vlines() - 1 ||
        (Region::PAL() && io.ly >= 264 && io.ly <= vlines() - 2)) {
      if (enable()) {
        ++sprite.oamMainCounterIndex;
        return;
      }
    }

    // The three unimplemented bits of each sprite's byte 2
    // do not exist in the PPU and always read back as 0 on
    // PPU revisions that allow reading PPU OAM through
    // OAMDATA ($2004)
    if (sprite.oamMainCounterTiming == 2)
      data.bit(2,4) = 0;

    oam[sprite.oamAddress] = data;
    ++sprite.oamMainCounter;
    break;

  //PPUSCROLL
  case 5:
    if(scroll.latch++ == 0) {
      scroll.fineX = data.bit(0,2);
      scroll.tileX = data.bit(3,7);

      if (oamScrollGlitch->value() && io.lx == 257 && rendering())
        var.tileX = cpu.io.openBus.bit(3,7);
    } else {
      scroll.fineY = data.bit(0,2);
      scroll.tileY = data.bit(3,7);
    }
    break;

  //PPUADDR
  case 6:
    if(scroll.latch++ == 0) {
      scroll.addressHi = data.bit(0,5);
      if (oamAddressGlitch->value() && io.lx == 257 && rendering())
        var.nametable = cpu.io.openBus.bit(2,3);
    } else {
      scroll.addressLo = data.bit(0,7);
      scroll.transferDelay = 3;
    }
    break;

  //PPUDATA
  case 7:
    address = io.busAddress;
    if (address >= 0x3f00)
      writeCGRAM(address, data);
    else
      cartridge.writeCHR(address, data);

    if (rendering()) {
      incrementVRAMAddressX();
      incrementVRAMAddressY();
    } else {
      var.address += io.vramIncrement;
      io.busAddress = var.address;
      cartridge.ppuAddressBus(io.busAddress);
    }
    break;

  }
}
