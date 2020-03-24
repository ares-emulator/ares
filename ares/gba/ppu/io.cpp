auto PPU::readIO(uint32 addr) -> uint8 {
  switch(addr) {

  //DISPCNT
  case 0x0400'0000: return (
    Background::IO::mode  << 0
  | io.gameBoyColorMode   << 3
  | Background::IO::frame << 4
  | objects.io.hblank     << 5
  | objects.io.mapping    << 6
  | io.forceBlank         << 7
  );
  case 0x0400'0001: return (
    bg0.io.enable     << 0
  | bg1.io.enable     << 1
  | bg2.io.enable     << 2
  | bg3.io.enable     << 3
  | objects.io.enable << 4
  | window0.io.enable << 5
  | window1.io.enable << 6
  | window2.io.enable << 7
  );

  //GRSWP
  case 0x0400'0002: return io.greenSwap;
  case 0x0400'0003: return 0;

  //DISPSTAT
  case 0x0400'0004: return (
    io.vblank          << 0
  | io.hblank          << 1
  | io.vcoincidence    << 2
  | io.irqvblank       << 3
  | io.irqhblank       << 4
  | io.irqvcoincidence << 5
  );
  case 0x0400'0005: return (
    io.vcompare
  );

  //VCOUNT
  case 0x0400'0006: return io.vcounter.byte(0);
  case 0x0400'0007: return io.vcounter.byte(1);

  //BG0CNT
  case 0x0400'0008: return bg0.io.priority << 0 | bg0.io.characterBase << 2 | bg0.io.unused << 4 | bg0.io.mosaic << 6 | bg0.io.colorMode << 7;
  case 0x0400'0009: return bg0.io.screenBase << 0 | bg0.io.affineWrap << 5 | bg0.io.screenSize << 6;

  //BG1CNT
  case 0x0400'000a: return bg1.io.priority << 0 | bg1.io.characterBase << 2 | bg1.io.unused << 4 | bg1.io.mosaic << 6 | bg1.io.colorMode << 7;
  case 0x0400'000b: return bg1.io.screenBase << 0 | bg1.io.affineWrap << 5 | bg1.io.screenSize << 6;

  //BG2CNT
  case 0x0400'000c: return bg2.io.priority << 0 | bg2.io.characterBase << 2 | bg2.io.unused << 4 | bg2.io.mosaic << 6 | bg2.io.colorMode << 7;
  case 0x0400'000d: return bg2.io.screenBase << 0 | bg2.io.affineWrap << 5 | bg2.io.screenSize << 6;

  //BG3CNT
  case 0x0400'000e: return bg3.io.priority << 0 | bg3.io.characterBase << 2 | bg3.io.unused << 4 | bg3.io.mosaic << 6 | bg3.io.colorMode << 7;
  case 0x0400'000f: return bg3.io.screenBase << 0 | bg3.io.affineWrap << 5 | bg3.io.screenSize << 6;

  //WININ0
  case 0x0400'0048: return window0.io.active[BG0] << 0 | window0.io.active[BG1] << 1 | window0.io.active[BG2] << 2 | window0.io.active[BG3] << 3 | window0.io.active[OBJ] << 4 | window0.io.active[SFX] << 5;

  //WININ1
  case 0x0400'0049: return window1.io.active[BG0] << 0 | window1.io.active[BG1] << 1 | window1.io.active[BG2] << 2 | window1.io.active[BG3] << 3 | window1.io.active[OBJ] << 4 | window1.io.active[SFX] << 5;

  //WINOUT
  case 0x0400'004a: return window3.io.active[BG0] << 0 | window3.io.active[BG1] << 1 | window3.io.active[BG2] << 2 | window3.io.active[BG3] << 3 | window3.io.active[OBJ] << 4 | window3.io.active[SFX] << 5;

  //WININ2
  case 0x0400'004b: return window2.io.active[BG0] << 0 | window2.io.active[BG1] << 1 | window2.io.active[BG2] << 2 | window2.io.active[BG3] << 3 | window2.io.active[OBJ] << 4 | window2.io.active[SFX] << 5;

  //MOSAIC (write-only)

  //BLTCNT
  case 0x0400'0050: return (
    dac.io.blendAbove[BG0] << 0
  | dac.io.blendAbove[BG1] << 1
  | dac.io.blendAbove[BG2] << 2
  | dac.io.blendAbove[BG3] << 3
  | dac.io.blendAbove[OBJ] << 4
  | dac.io.blendAbove[SFX] << 5
  | dac.io.blendMode       << 6
  );
  case 0x0400'0051: return (
    dac.io.blendBelow[BG0] << 0
  | dac.io.blendBelow[BG1] << 1
  | dac.io.blendBelow[BG2] << 2
  | dac.io.blendBelow[BG3] << 3
  | dac.io.blendBelow[OBJ] << 4
  | dac.io.blendBelow[SFX] << 5
  );

  //BLDALPHA
  case 0x0400'0052: return dac.io.blendEVA;
  case 0x0400'0053: return dac.io.blendEVB;

  //BLDY (write-only)

  }

  return cpu.pipeline.fetch.instruction.byte(addr & 1);
}

auto PPU::writeIO(uint32 addr, uint8 data) -> void {
  switch(addr) {

  //DISPCNT
  case 0x0400'0000:
    Background::IO::mode  = data.bit(0,2);
    io.gameBoyColorMode   = data.bit(3);
    Background::IO::frame = data.bit(4);
    objects.io.hblank     = data.bit(5);
    objects.io.mapping    = data.bit(6);
    io.forceBlank         = data.bit(7);
    return;
  case 0x0400'0001:
    bg0.io.enable     = data.bit(0);
    bg1.io.enable     = data.bit(1);
    bg2.io.enable     = data.bit(2);
    bg3.io.enable     = data.bit(3);
    objects.io.enable = data.bit(4);
    window0.io.enable = data.bit(5);
    window1.io.enable = data.bit(6);
    window2.io.enable = data.bit(7);
    //outside window is enabled whenever any inside window is enabled
    window3.io.enable = (bool)data.bit(5,7);
    return;

  //GRSWP
  case 0x0400'0002:
    io.greenSwap = data.bit(0);
    return;
  case 0x0400'0003:
    return;

  //DISPSTAT
  case 0x0400'0004:
    io.irqvblank       = data.bit(3);
    io.irqhblank       = data.bit(4);
    io.irqvcoincidence = data.bit(5);
    return;
  case 0x0400'0005:
    io.vcompare = data;
    return;

  //BG0CNT
  case 0x0400'0008:
    bg0.io.priority      = data.bit(0,1);
    bg0.io.characterBase = data.bit(2,3);
    bg0.io.unused        = data.bit(4,5);
    bg0.io.mosaic        = data.bit(6);
    bg0.io.colorMode     = data.bit(7);
    return;
  case 0x0400'0009:
    bg0.io.screenBase    = data.bit(0,4);
    bg0.io.affineWrap    = false;
    bg0.io.screenSize    = data.bit(6,7);
    return;

  //BG1CNT
  case 0x0400'000a:
    bg1.io.priority      = data.bit(0,1);
    bg1.io.characterBase = data.bit(2,3);
    bg1.io.unused        = data.bit(4,5);
    bg1.io.mosaic        = data.bit(6);
    bg1.io.colorMode     = data.bit(7);
    return;
  case 0x0400'000b:
    bg1.io.screenBase    = data.bit(0,4);
    bg1.io.affineWrap    = false;
    bg1.io.screenSize    = data.bit(6,7);
    return;

  //BG2CNT
  case 0x0400'000c:
    bg2.io.priority      = data.bit(0,1);
    bg2.io.characterBase = data.bit(2,3);
    bg2.io.unused        = data.bit(4,5);
    bg2.io.mosaic        = data.bit(6);
    bg2.io.colorMode     = data.bit(7);
    return;
  case 0x0400'000d:
    bg2.io.screenBase    = data.bit(0,4);
    bg2.io.affineWrap    = data.bit(5);
    bg2.io.screenSize    = data.bit(6,7);
    return;

  //BG3CNT
  case 0x0400'000e:
    bg3.io.priority      = data.bit(0,1);
    bg3.io.characterBase = data.bit(2,3);
    bg3.io.unused        = data.bit(4,5);
    bg3.io.mosaic        = data.bit(6);
    bg3.io.colorMode     = data.bit(7);
    return;
  case 0x0400'000f:
    bg3.io.screenBase    = data.bit(0,4);
    bg3.io.affineWrap    = data.bit(5);
    bg3.io.screenSize    = data.bit(6,7);
    return;

  //BG0HOFS
  case 0x0400'0010: bg0.io.hoffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'0011: bg0.io.hoffset.bit(8)   = data.bit(0);   return;

  //BG0VOFS
  case 0x0400'0012: bg0.io.voffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'0013: bg0.io.voffset.bit(8)   = data.bit(0);   return;

  //BG1HOFS
  case 0x0400'0014: bg1.io.hoffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'0015: bg1.io.hoffset.bit(8)   = data.bit(0);   return;

  //BG1VOFS
  case 0x0400'0016: bg1.io.voffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'0017: bg1.io.voffset.bit(8)   = data.bit(0);   return;

  //BG2HOFS
  case 0x0400'0018: bg2.io.hoffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'0019: bg2.io.hoffset.bit(8)   = data.bit(0);   return;

  //BG2VOFS
  case 0x0400'001a: bg2.io.voffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'001b: bg2.io.voffset.bit(8)   = data.bit(0);   return;

  //BG3HOFS
  case 0x0400'001c: bg3.io.hoffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'001d: bg3.io.hoffset.bit(8)   = data.bit(0);   return;

  //BG3VOFS
  case 0x0400'001e: bg3.io.voffset.bit(0,7) = data.bit(0,7); return;
  case 0x0400'001f: bg3.io.voffset.bit(8)   = data.bit(0);   return;

  //BG2PA
  case 0x0400'0020: bg2.io.pa.byte(0) = data; return;
  case 0x0400'0021: bg2.io.pa.byte(1) = data; return;

  //BG2PB
  case 0x0400'0022: bg2.io.pb.byte(0) = data; return;
  case 0x0400'0023: bg2.io.pb.byte(1) = data; return;

  //BG2PC
  case 0x0400'0024: bg2.io.pc.byte(0) = data; return;
  case 0x0400'0025: bg2.io.pc.byte(1) = data; return;

  //BG2PD
  case 0x0400'0026: bg2.io.pd.byte(0) = data; return;
  case 0x0400'0027: bg2.io.pd.byte(1) = data; return;

  //BG2X
  case 0x0400'0028: bg2.io.x.bit( 0, 7) = data.bit(0,7); bg2.io.lx = bg2.io.x; return;
  case 0x0400'0029: bg2.io.x.bit( 8,15) = data.bit(0,7); bg2.io.lx = bg2.io.x; return;
  case 0x0400'002a: bg2.io.x.bit(16,23) = data.bit(0,7); bg2.io.lx = bg2.io.x; return;
  case 0x0400'002b: bg2.io.x.bit(24,27) = data.bit(0,3); bg2.io.lx = bg2.io.x; return;

  //BG2Y
  case 0x0400'002c: bg2.io.y.bit( 0, 7) = data.bit(0,7); bg2.io.ly = bg2.io.y; return;
  case 0x0400'002d: bg2.io.y.bit( 8,15) = data.bit(0,7); bg2.io.ly = bg2.io.y; return;
  case 0x0400'002e: bg2.io.y.bit(16,23) = data.bit(0,7); bg2.io.ly = bg2.io.y; return;
  case 0x0400'002f: bg2.io.y.bit(24,27) = data.bit(0,3); bg2.io.ly = bg2.io.y; return;

  //BG3PA
  case 0x0400'0030: bg3.io.pa.byte(0) = data; return;
  case 0x0400'0031: bg3.io.pa.byte(1) = data; return;

  //BG3PB
  case 0x0400'0032: bg3.io.pb.byte(0) = data; return;
  case 0x0400'0033: bg3.io.pb.byte(1) = data; return;

  //BG3PC
  case 0x0400'0034: bg3.io.pc.byte(0) = data; return;
  case 0x0400'0035: bg3.io.pc.byte(1) = data; return;

  //BG3PD
  case 0x0400'0036: bg3.io.pd.byte(0) = data; return;
  case 0x0400'0037: bg3.io.pd.byte(1) = data; return;

  //BG3X
  case 0x0400'0038: bg3.io.x.bit( 0, 7) = data.bit(0,7); bg3.io.lx = bg3.io.x; return;
  case 0x0400'0039: bg3.io.x.bit( 8,15) = data.bit(0,7); bg3.io.lx = bg3.io.x; return;
  case 0x0400'003a: bg3.io.x.bit(16,23) = data.bit(0,7); bg3.io.lx = bg3.io.x; return;
  case 0x0400'003b: bg3.io.x.bit(24,27) = data.bit(0,3); bg3.io.lx = bg3.io.x; return;

  //BG3Y
  case 0x0400'003c: bg3.io.y.bit( 0, 7) = data.bit(0,7); bg3.io.ly = bg3.io.y; return;
  case 0x0400'003d: bg3.io.y.bit( 8,15) = data.bit(0,7); bg3.io.ly = bg3.io.y; return;
  case 0x0400'003e: bg3.io.y.bit(16,23) = data.bit(0,7); bg3.io.ly = bg3.io.y; return;
  case 0x0400'003f: bg3.io.y.bit(24,27) = data.bit(0,3); bg3.io.ly = bg3.io.y; return;

  //WIN0H
  case 0x0400'0040: window0.io.x2 = data; return;
  case 0x0400'0041: window0.io.x1 = data; return;

  //WIN1H
  case 0x0400'0042: window1.io.x2 = data; return;
  case 0x0400'0043: window1.io.x1 = data; return;

  //WIN0V
  case 0x0400'0044: window0.io.y2 = data; return;
  case 0x0400'0045: window0.io.y1 = data; return;

  //WIN1V
  case 0x0400'0046: window1.io.y2 = data; return;
  case 0x0400'0047: window1.io.y1 = data; return;

  //WININ0
  case 0x0400'0048:
    window0.io.active[BG0] = data.bit(0);
    window0.io.active[BG1] = data.bit(1);
    window0.io.active[BG2] = data.bit(2);
    window0.io.active[BG3] = data.bit(3);
    window0.io.active[OBJ] = data.bit(4);
    window0.io.active[SFX] = data.bit(5);
    return;

  //WININ1
  case 0x0400'0049:
    window1.io.active[BG0] = data.bit(0);
    window1.io.active[BG1] = data.bit(1);
    window1.io.active[BG2] = data.bit(2);
    window1.io.active[BG3] = data.bit(3);
    window1.io.active[OBJ] = data.bit(4);
    window1.io.active[SFX] = data.bit(5);
    return;

  //WINOUT
  case 0x0400'004a:
    window3.io.active[BG0] = data.bit(0);
    window3.io.active[BG1] = data.bit(1);
    window3.io.active[BG2] = data.bit(2);
    window3.io.active[BG3] = data.bit(3);
    window3.io.active[OBJ] = data.bit(4);
    window3.io.active[SFX] = data.bit(5);
    return;

  //WININ2
  case 0x0400'004b:
    window2.io.active[BG0] = data.bit(0);
    window2.io.active[BG1] = data.bit(1);
    window2.io.active[BG2] = data.bit(2);
    window2.io.active[BG3] = data.bit(3);
    window2.io.active[OBJ] = data.bit(4);
    window2.io.active[SFX] = data.bit(5);
    return;

  //MOSAIC
  case 0x0400'004c:
    Background::IO::mosaicWidth  = data.bit(0,3);
    Background::IO::mosaicHeight = data.bit(4,7);
    return;
  case 0x0400'004d:
    objects.io.mosaicWidth  = data.bit(0,3);
    objects.io.mosaicHeight = data.bit(4,7);
    return;

  //BLDCNT
  case 0x0400'0050:
    dac.io.blendAbove[BG0] = data.bit(0);
    dac.io.blendAbove[BG1] = data.bit(1);
    dac.io.blendAbove[BG2] = data.bit(2);
    dac.io.blendAbove[BG3] = data.bit(3);
    dac.io.blendAbove[OBJ] = data.bit(4);
    dac.io.blendAbove[SFX] = data.bit(5);
    dac.io.blendMode       = data.bit(6,7);
    return;
  case 0x0400'0051:
    dac.io.blendBelow[BG0] = data.bit(0);
    dac.io.blendBelow[BG1] = data.bit(1);
    dac.io.blendBelow[BG2] = data.bit(2);
    dac.io.blendBelow[BG3] = data.bit(3);
    dac.io.blendBelow[OBJ] = data.bit(4);
    dac.io.blendBelow[SFX] = data.bit(5);
    return;

  //BLDALPHA
  case 0x0400'0052: dac.io.blendEVA = data.bit(0,4); return;
  case 0x0400'0053: dac.io.blendEVB = data.bit(0,4); return;

  //BLDY
  case 0x0400'0054: dac.io.blendEVY = data.bit(0,4); return;
  case 0x0400'0055: return;

  }
}
