auto PPU::latchCounters() -> void {
  cpu.synchronize(ppu);
  io.hcounter = cpu.hdot();
  io.vcounter = cpu.vcounter();
  latch.counters = 1;
}

alwaysinline auto PPU::addressVRAM() const -> uint16 {
  uint16 address = vram.address;
  switch(vram.mapping) {
  case 0: return address;
  case 1: return address.bit( 8,15) <<  8 | address.bit(0,4) << 3 | address.bit(5,7);
  case 2: return address.bit( 9,15) <<  9 | address.bit(0,5) << 3 | address.bit(6,8);
  case 3: return address.bit(10,15) << 10 | address.bit(0,6) << 3 | address.bit(7,9);
  }
  unreachable;
}

alwaysinline auto PPU::readVRAM() -> uint16 {
  if(!io.displayDisable && cpu.vcounter() < vdisp()) return 0x0000;
  auto address = addressVRAM();
  return vram[address];
}

alwaysinline auto PPU::writeVRAM(uint1 byte, uint8 data) -> void {
  if(!io.displayDisable && cpu.vcounter() < vdisp()) return;
  auto address = addressVRAM();
  vram[address].byte(byte) = data;
}

alwaysinline auto PPU::readOAM(uint10 address) -> uint8 {
  if(!io.displayDisable && cpu.vcounter() < vdisp()) address = 0x0218;
  return obj.oam.read(address);
}

alwaysinline auto PPU::writeOAM(uint10 address, uint8 data) -> void {
  if(!io.displayDisable && cpu.vcounter() < vdisp()) address = 0x0218;
  return obj.oam.write(address, data);
}

alwaysinline auto PPU::readCGRAM(uint1 byte, uint8 address) -> uint8 {
  if(!io.displayDisable
  && cpu.vcounter() > 0 && cpu.vcounter() < vdisp()
  && cpu.hcounter() >= 88 && cpu.hcounter() < 1096
  ) address = latch.cgramAddress;
  return dac.cgram[address].byte(byte);
}

alwaysinline auto PPU::writeCGRAM(uint8 address, uint15 data) -> void {
  if(!io.displayDisable
  && cpu.vcounter() > 0 && cpu.vcounter() < vdisp()
  && cpu.hcounter() >= 88 && cpu.hcounter() < 1096
  ) address = latch.cgramAddress;
  dac.cgram[address] = data;
}

auto PPU::readIO(uint24 address, uint8 data) -> uint8 {
  cpu.synchronize(ppu);

  switch((uint16)address) {

  case 0x2104: case 0x2105: case 0x2106: case 0x2108:
  case 0x2109: case 0x210a: case 0x2114: case 0x2115:
  case 0x2116: case 0x2118: case 0x2119: case 0x211a:
  case 0x2124: case 0x2125: case 0x2126: case 0x2128:
  case 0x2129: case 0x212a: {
    return ppu1.mdr;
  }

  //MPYL
  case 0x2134: {
    uint24 result = (int16)mode7.a * (int8)(mode7.b >> 8);
    return ppu1.mdr = result.byte(0);
  }

  //MPYM
  case 0x2135: {
    uint24 result = (int16)mode7.a * (int8)(mode7.b >> 8);
    return ppu1.mdr = result.byte(1);
  }

  //MYPH
  case 0x2136: {
    uint24 result = (int16)mode7.a * (int8)(mode7.b >> 8);
    return ppu1.mdr = result.byte(2);
  }

  //SLHV
  case 0x2137: {
    if(cpu.pio().bit(7)) latchCounters();
    return data;  //CPU MDR
  }

  //OAMDATAREAD
  case 0x2138: {
    ppu1.mdr = readOAM(io.oamAddress++);
    obj.setFirstSprite();
    return ppu1.mdr;
  }

  //VMDATALREAD
  case 0x2139: {
    ppu1.mdr = latch.vram.byte(0);
    if(vram.mode == 0) {
      latch.vram = readVRAM();
      vram.address += vram.increment;
    }
    return ppu1.mdr;
  }

  //VMDATAHREAD
  case 0x213a: {
    ppu1.mdr = latch.vram.byte(1);
    if(vram.mode == 1) {
      latch.vram = readVRAM();
      vram.address += vram.increment;
    }
    return ppu1.mdr;
  }

  //CGDATAREAD
  case 0x213b: {
    if(io.cgramAddressLatch++ == 0) {
      ppu2.mdr.bit(0,7) = readCGRAM(0, io.cgramAddress);
    } else {
      ppu2.mdr.bit(0,6) = readCGRAM(1, io.cgramAddress++);
    }
    return ppu2.mdr;
  }

  //OPHCT
  case 0x213c: {
    if(latch.hcounter++ == 0) {
      ppu2.mdr.bit(0,7) = io.hcounter.bit(0,7);
    } else {
      ppu2.mdr.bit(0)   = io.hcounter.bit(8);
    }
    return ppu2.mdr;
  }

  //OPVCT
  case 0x213d: {
    if(latch.vcounter++ == 0) {
      ppu2.mdr.bit(0,7) = io.vcounter.bit(0,7);
    } else {
      ppu2.mdr.bit(0)   = io.vcounter.bit(8);
    }
    return ppu2.mdr;
  }

  //STAT77
  case 0x213e: {
    ppu1.mdr.bit(0,3) = ppu1.version;
    ppu1.mdr.bit(5)   = 0;
    ppu1.mdr.bit(6)   = obj.io.rangeOver;
    ppu1.mdr.bit(7)   = obj.io.timeOver;
    return ppu1.mdr;
  }

  //STAT78
  case 0x213f: {
    latch.hcounter = 0;
    latch.vcounter = 0;
    ppu2.mdr.bit(0,3) = ppu2.version;
    ppu2.mdr.bit(4)   = Region::PAL();  //0 = NTSC, 1 = PAL
    if(!cpu.pio().bit(7)) {
      ppu2.mdr.bit(6) = 1;
    } else {
      ppu2.mdr.bit(6) = latch.counters;
      latch.counters = 0;
    }
    ppu2.mdr.bit(7) = cpu.field();
    return ppu2.mdr;
  }

  }

  return data;
}

auto PPU::writeIO(uint24 address, uint8 data) -> void {
  cpu.synchronize(ppu);

  switch((uint16)address) {

  //INIDISP
  case 0x2100: {
    if(io.displayDisable && vcounter() == vdisp()) obj.addressReset();
    io.displayBrightness = data.bit(0,3);
    io.displayDisable    = data.bit(7);
    return;
  }

  //OBSEL
  case 0x2101: {
    obj.io.tiledataAddress = data.bit(0,2) << 13;
    obj.io.nameselect      = data.bit(3,4);
    obj.io.baseSize        = data.bit(5,7);
    return;
  }

  //OAMADDL
  case 0x2102: {
    io.oamBaseAddress.bit(1,8) = data.bit(0,7);
    obj.addressReset();
    return;
  }

  //OAMADDH
  case 0x2103: {
    io.oamBaseAddress.bit(9) = data.bit(0);
    io.oamPriority = data.bit(7);
    obj.addressReset();
    return;
  }

  //OAMDATA
  case 0x2104: {
    uint1 latchBit = io.oamAddress.bit(0);
    uint10 address = io.oamAddress++;
    if(latchBit == 0) latch.oam = data;
    if(address.bit(9)) {
      writeOAM(address, data);
    } else if(latchBit == 1) {
      writeOAM((address & ~1) + 0, latch.oam);
      writeOAM((address & ~1) + 1, data);
    }
    obj.setFirstSprite();
    return;
  }

  //BGMODE
  case 0x2105: {
    io.bgMode       = data.bit(0,2);
    io.bgPriority   = data.bit(3);
    bg1.io.tileSize = data.bit(4);
    bg2.io.tileSize = data.bit(5);
    bg3.io.tileSize = data.bit(6);
    bg4.io.tileSize = data.bit(7);
    updateVideoMode();
    return;
  }

  //MOSAIC
  case 0x2106: {
    bool mosaicEnable   = mosaic.enable();
    bg1.io.mosaicEnable = data.bit(0);
    bg2.io.mosaicEnable = data.bit(1);
    bg3.io.mosaicEnable = data.bit(2);
    bg4.io.mosaicEnable = data.bit(3);
    mosaic.size         = data.bit(4,7) + 1;
    if(!mosaicEnable && mosaic.enable()) {
      //mosaic vcounter is reloaded when mosaic becomes enabled
      mosaic.vcounter = mosaic.size + 1;
    }
    return;
  }

  //BG1SC
  case 0x2107: {
    bg1.io.screenSize    = data.bit(0,1);
    bg1.io.screenAddress = data.bit(2,7) << 10;
    return;
  }

  //BG2SC
  case 0x2108: {
    bg2.io.screenSize    = data.bit(0,1);
    bg2.io.screenAddress = data.bit(2,7) << 10;
    return;
  }

  //BG3SC
  case 0x2109: {
    bg3.io.screenSize    = data.bit(0,1);
    bg3.io.screenAddress = data.bit(2,7) << 10;
    return;
  }

  //BG4SC
  case 0x210a: {
    bg4.io.screenSize    = data.bit(0,1);
    bg4.io.screenAddress = data.bit(2,7) << 10;
    return;
  }

  //BG12NBA
  case 0x210b: {
    bg1.io.tiledataAddress = data.bit(0,3) << 12;
    bg2.io.tiledataAddress = data.bit(4,7) << 12;
    return;
  }

  //BG34NBA
  case 0x210c: {
    bg3.io.tiledataAddress = data.bit(0,3) << 12;
    bg4.io.tiledataAddress = data.bit(4,7) << 12;
    return;
  }

  //BG1HOFS
  case 0x210d: {
    mode7.hoffset = data << 8 | latch.mode7;
    latch.mode7 = data;

    bg1.io.hoffset = data << 8 | latch.bgofsPPU1 & ~7 | latch.bgofsPPU2 & 7;
    latch.bgofsPPU1 = data;
    latch.bgofsPPU2 = data;
    return;
  }

  //BG1VOFS
  case 0x210e: {
    mode7.voffset = data << 8 | latch.mode7;
    latch.mode7 = data;

    bg1.io.voffset = data << 8 | latch.bgofsPPU1;
    latch.bgofsPPU1 = data;
    return;
  }

  //BG2HOFS
  case 0x210f: {
    bg2.io.hoffset = data << 8 | latch.bgofsPPU1 & ~7 | latch.bgofsPPU2 & 7;
    latch.bgofsPPU1 = data;
    latch.bgofsPPU2 = data;
    return;
  }

  //BG2VOFS
  case 0x2110: {
    bg2.io.voffset = data << 8 | latch.bgofsPPU1;
    latch.bgofsPPU1 = data;
    return;
  }

  //BG3HOFS
  case 0x2111: {
    bg3.io.hoffset = data << 8 | latch.bgofsPPU1 & ~7 | latch.bgofsPPU2 & 7;
    latch.bgofsPPU1 = data;
    latch.bgofsPPU2 = data;
    return;
  }

  //BG3VOFS
  case 0x2112: {
    bg3.io.voffset = data << 8 | latch.bgofsPPU1;
    latch.bgofsPPU1 = data;
    return;
  }

  //BG4HOFS
  case 0x2113: {
    bg4.io.hoffset = data << 8 | latch.bgofsPPU1 & ~7 | latch.bgofsPPU2 & 7;
    latch.bgofsPPU1 = data;
    latch.bgofsPPU2 = data;
    return;
  }

  //BG4VOFS
  case 0x2114: {
    bg4.io.voffset = data << 8 | latch.bgofsPPU1;
    latch.bgofsPPU1 = data;
    return;
  }

  //VMAIN
  case 0x2115: {
    static const uint size[4] = {1, 32, 128, 128};
    vram.increment = size[data.bit(0,1)];
    vram.mapping   = data.bit(2,3);
    vram.mode      = data.bit(7);
    return;
  }

  //VMADDL
  case 0x2116: {
    vram.address.bit(0,7) = data;
    latch.vram = readVRAM();
    return;
  }

  //VHADDH
  case 0x2117: {
    vram.address.bit(8,15) = data;
    latch.vram = readVRAM();
    return;
  }

  //VMDATAL
  case 0x2118: {
    writeVRAM(0, data);
    if(vram.mode == 0) vram.address += vram.increment;
    return;
  }

  //VMDATAH
  case 0x2119: {
    writeVRAM(1, data);
    if(vram.mode == 1) vram.address += vram.increment;
    return;
  }

  //M7SEL
  case 0x211a: {
    mode7.hflip  = data.bit(0);
    mode7.vflip  = data.bit(1);
    mode7.repeat = data.bit(6,7);
    return;
  }

  //M7A
  case 0x211b: {
    mode7.a = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //M7B
  case 0x211c: {
    mode7.b = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //M7C
  case 0x211d: {
    mode7.c = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //M7D
  case 0x211e: {
    mode7.d = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //M7X
  case 0x211f: {
    mode7.hcenter = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //M7Y
  case 0x2120: {
    mode7.vcenter = data << 8 | latch.mode7;
    latch.mode7 = data;
    return;
  }

  //CGADD
  case 0x2121: {
    io.cgramAddress = data;
    io.cgramAddressLatch = 0;
    return;
  }

  //CGDATA
  case 0x2122: {
    if(io.cgramAddressLatch++ == 0) {
      latch.cgram = data;
    } else {
      writeCGRAM(io.cgramAddress++, data.bit(0,6) << 8 | latch.cgram);
    }
    return;
  }

  //W12SEL
  case 0x2123: {
    bg1.window.oneInvert = data.bit(0);
    bg1.window.oneEnable = data.bit(1);
    bg1.window.twoInvert = data.bit(2);
    bg1.window.twoEnable = data.bit(3);
    bg2.window.oneInvert = data.bit(4);
    bg2.window.oneEnable = data.bit(5);
    bg2.window.twoInvert = data.bit(6);
    bg2.window.twoEnable = data.bit(7);
    return;
  }

  //W34SEL
  case 0x2124: {
    bg3.window.oneInvert = data.bit(0);
    bg3.window.oneEnable = data.bit(1);
    bg3.window.twoInvert = data.bit(2);
    bg3.window.twoEnable = data.bit(3);
    bg4.window.oneInvert = data.bit(4);
    bg4.window.oneEnable = data.bit(5);
    bg4.window.twoInvert = data.bit(6);
    bg4.window.twoEnable = data.bit(7);
    return;
  }

  //WOBJSEL
  case 0x2125: {
    obj.window.oneInvert = data.bit(0);
    obj.window.oneEnable = data.bit(1);
    obj.window.twoInvert = data.bit(2);
    obj.window.twoEnable = data.bit(3);
    dac.window.oneInvert = data.bit(4);
    dac.window.oneEnable = data.bit(5);
    dac.window.twoInvert = data.bit(6);
    dac.window.twoEnable = data.bit(7);
    return;
  }

  //WH0
  case 0x2126: {
    window.io.oneLeft = data;
    return;
  }

  //WH1
  case 0x2127: {
    window.io.oneRight = data;
    return;
  }

  //WH2
  case 0x2128: {
    window.io.twoLeft = data;
    return;
  }

  //WH3
  case 0x2129: {
    window.io.twoRight = data;
    return;
  }

  //WBGLOG
  case 0x212a: {
    bg1.window.mask = data.bit(0,1);
    bg2.window.mask = data.bit(2,3);
    bg3.window.mask = data.bit(4,5);
    bg4.window.mask = data.bit(6,7);
    return;
  }

  //WOBJLOG
  case 0x212b: {
    obj.window.mask = data.bit(0,1);
    dac.window.mask = data.bit(2,3);
    return;
  }

  //TM
  case 0x212c: {
    bg1.io.aboveEnable = data.bit(0);
    bg2.io.aboveEnable = data.bit(1);
    bg3.io.aboveEnable = data.bit(2);
    bg4.io.aboveEnable = data.bit(3);
    obj.io.aboveEnable = data.bit(4);
    return;
  }

  //TS
  case 0x212d: {
    bg1.io.belowEnable = data.bit(0);
    bg2.io.belowEnable = data.bit(1);
    bg3.io.belowEnable = data.bit(2);
    bg4.io.belowEnable = data.bit(3);
    obj.io.belowEnable = data.bit(4);
    return;
  }

  //TMW
  case 0x212e: {
    bg1.window.aboveEnable = data.bit(0);
    bg2.window.aboveEnable = data.bit(1);
    bg3.window.aboveEnable = data.bit(2);
    bg4.window.aboveEnable = data.bit(3);
    obj.window.aboveEnable = data.bit(4);
    return;
  }

  //TSW
  case 0x212f: {
    bg1.window.belowEnable = data.bit(0);
    bg2.window.belowEnable = data.bit(1);
    bg3.window.belowEnable = data.bit(2);
    bg4.window.belowEnable = data.bit(3);
    obj.window.belowEnable = data.bit(4);
    return;
  }

  //CGWSEL
  case 0x2130: {
    dac.io.directColor   = data.bit(0);
    dac.io.blendMode     = data.bit(1);
    dac.window.belowMask = data.bit(4,5);
    dac.window.aboveMask = data.bit(6,7);
    return;
  }

  //CGADDSUB
  case 0x2131: {
    dac.io.colorEnable[Source::BG1 ] = data.bit(0);
    dac.io.colorEnable[Source::BG2 ] = data.bit(1);
    dac.io.colorEnable[Source::BG3 ] = data.bit(2);
    dac.io.colorEnable[Source::BG4 ] = data.bit(3);
    dac.io.colorEnable[Source::OBJ1] = 0;
    dac.io.colorEnable[Source::OBJ2] = data.bit(4);
    dac.io.colorEnable[Source::COL ] = data.bit(5);
    dac.io.colorHalve = data.bit(6);
    dac.io.colorMode  = data.bit(7);
    return;
  }

  //COLDATA
  case 0x2132: {
    if(data.bit(5)) dac.io.colorRed   = data.bit(0,4);
    if(data.bit(6)) dac.io.colorGreen = data.bit(0,4);
    if(data.bit(7)) dac.io.colorBlue  = data.bit(0,4);
    return;
  }

  //SETINI
  case 0x2133: {
    io.interlace     = data.bit(0);
    obj.io.interlace = data.bit(1);
    io.overscan      = data.bit(2);
    io.pseudoHires   = data.bit(3);
    io.extbg         = data.bit(6);
    updateVideoMode();
    return;
  }

  }
}

auto PPU::updateVideoMode() -> void {
  state.vdisp = !io.overscan ? 225 : 240;

  switch(io.bgMode) {
  case 0:
    bg1.io.mode = Background::Mode::BPP2;
    bg2.io.mode = Background::Mode::BPP2;
    bg3.io.mode = Background::Mode::BPP2;
    bg4.io.mode = Background::Mode::BPP2;
    memory::assign(bg1.io.priority, 8, 11);
    memory::assign(bg2.io.priority, 7, 10);
    memory::assign(bg3.io.priority, 2,  5);
    memory::assign(bg4.io.priority, 1,  4);
    memory::assign(obj.io.priority, 3,  6, 9, 12);
    break;

  case 1:
    bg1.io.mode = Background::Mode::BPP4;
    bg2.io.mode = Background::Mode::BPP4;
    bg3.io.mode = Background::Mode::BPP2;
    bg4.io.mode = Background::Mode::Inactive;
    if(io.bgPriority) {
      memory::assign(bg1.io.priority, 5,  8);
      memory::assign(bg2.io.priority, 4,  7);
      memory::assign(bg3.io.priority, 1, 10);
      memory::assign(obj.io.priority, 2,  3, 6,  9);
    } else {
      memory::assign(bg1.io.priority, 6,  9);
      memory::assign(bg2.io.priority, 5,  8);
      memory::assign(bg3.io.priority, 1,  3);
      memory::assign(obj.io.priority, 2,  4, 7, 10);
    }
    break;

  case 2:
    bg1.io.mode = Background::Mode::BPP4;
    bg2.io.mode = Background::Mode::BPP4;
    bg3.io.mode = Background::Mode::Inactive;
    bg4.io.mode = Background::Mode::Inactive;
    memory::assign(bg1.io.priority, 3, 7);
    memory::assign(bg2.io.priority, 1, 5);
    memory::assign(obj.io.priority, 2, 4, 6, 8);
    break;

  case 3:
    bg1.io.mode = Background::Mode::BPP8;
    bg2.io.mode = Background::Mode::BPP4;
    bg3.io.mode = Background::Mode::Inactive;
    bg4.io.mode = Background::Mode::Inactive;
    memory::assign(bg1.io.priority, 3, 7);
    memory::assign(bg2.io.priority, 1, 5);
    memory::assign(obj.io.priority, 2, 4, 6, 8);
    break;

  case 4:
    bg1.io.mode = Background::Mode::BPP8;
    bg2.io.mode = Background::Mode::BPP2;
    bg3.io.mode = Background::Mode::Inactive;
    bg4.io.mode = Background::Mode::Inactive;
    memory::assign(bg1.io.priority, 3, 7);
    memory::assign(bg2.io.priority, 1, 5);
    memory::assign(obj.io.priority, 2, 4, 6, 8);
    break;

  case 5:
    bg1.io.mode = Background::Mode::BPP4;
    bg2.io.mode = Background::Mode::BPP2;
    bg3.io.mode = Background::Mode::Inactive;
    bg4.io.mode = Background::Mode::Inactive;
    memory::assign(bg1.io.priority, 3, 7);
    memory::assign(bg2.io.priority, 1, 5);
    memory::assign(obj.io.priority, 2, 4, 6, 8);
    break;

  case 6:
    bg1.io.mode = Background::Mode::BPP4;
    bg2.io.mode = Background::Mode::Inactive;
    bg3.io.mode = Background::Mode::Inactive;
    bg4.io.mode = Background::Mode::Inactive;
    memory::assign(bg1.io.priority, 2, 5);
    memory::assign(obj.io.priority, 1, 3, 4, 6);
    break;

  case 7:
    if(!io.extbg) {
      bg1.io.mode = Background::Mode::Mode7;
      bg2.io.mode = Background::Mode::Inactive;
      bg3.io.mode = Background::Mode::Inactive;
      bg4.io.mode = Background::Mode::Inactive;
      memory::assign(bg1.io.priority, 2);
      memory::assign(obj.io.priority, 1, 3, 4, 5);
    } else {
      bg1.io.mode = Background::Mode::Mode7;
      bg2.io.mode = Background::Mode::Mode7;
      bg3.io.mode = Background::Mode::Inactive;
      bg4.io.mode = Background::Mode::Inactive;
      memory::assign(bg1.io.priority, 3);
      memory::assign(bg2.io.priority, 1, 5);
      memory::assign(obj.io.priority, 2, 4, 6, 7);
    }
    break;
  }
}
