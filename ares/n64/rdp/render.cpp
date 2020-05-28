static const vector<string> commandNames = {
  "No_Operation", "Invalid_01", "Invalid_02", "Invalid_03",
  "Invalid_04",   "Invalid_05", "Invalid_06", "Invalid_07",
  "Unshaded_Triangle",
  "Unshaded_Zbuffer_Triangle",
  "Texture_Triangle",
  "Texture_Zbuffer_Triangle",
  "Shaded_Triangle",
  "Shaded_Zbuffer_Triangle",
  "Shaded_Texture_Triangle",
  "Shaded_Texture_Zbuffer_Triangle",
  "Invalid_10", "Invalid_11", "Invalid_12", "Invalid_13",
  "Invalid_14", "Invalid_15", "Invalid_16", "Invalid_17",
  "Invalid_18", "Invalid_19", "Invalid_1a", "Invalid_1b",
  "Invalid_1c", "Invalid_1d", "Invalid_1e", "Invalid_1f",
  "Invalid_20", "Invalid_21", "Invalid_22", "Invalid_23",
  "Texture_Rectangle",
  "Texture_Rectangle_Flip",
  "Sync_Load",
  "Sync_Pipe",
  "Sync_Tile",
  "Sync_Full",
  "Set_Key_GB",
  "Set_Key_R",
  "Set_Convert",
  "Set_Scissor",
  "Set_Primitive_Depth",
  "Set_Other_Modes",
  "Load_Texture_LUT",
  "Invalid_31",
  "Set_Tile_Size",
  "Load_Block",
  "Load_Tile",
  "Set_Tile",
  "Fill_Rectangle",
  "Set_Fill_Color",
  "Set_Fog_Color",
  "Set_Blend_Color",
  "Set_Primitive_Color",
  "Set_Environment_Color",
  "Set_Combine_Mode",
  "Set_Texture_Image",
  "Set_Mask_Image",
  "Set_Color_Image",
};

auto RDP::render() -> void {
  auto& memory = !command.source ? rdram.ram : rsp.dmem;

  auto fetch = [&]() -> u64 {
    u64 op = memory.readDoubleUnaligned(command.current);
    command.current += 8;
    return op;
  };

  auto fetchEdge = [&](u64 op) {
    edge.lmajor   =  uint1(op >> 55);
    edge.level    =  uint3(op >> 51);
    edge.tile     =  uint3(op >> 48);
    edge.y.lo     = uint14(op >> 32);
    edge.y.md     = uint14(op >> 16);
    edge.y.hi     = uint14(op >>  0);
    op = fetch();
    edge.x.lo.c.i = uint16(op >> 48);
    edge.x.lo.c.f = uint16(op >> 32);
    edge.x.lo.s.i = uint16(op >> 16);
    edge.x.lo.s.f = uint16(op >>  0);
    op = fetch();
    edge.x.hi.c.i = uint16(op >> 48);
    edge.x.hi.c.f = uint16(op >> 32);
    edge.x.hi.s.i = uint16(op >> 16);
    edge.x.hi.s.f = uint16(op >>  0);
    op = fetch();
    edge.x.md.c.i = uint16(op >> 48);
    edge.x.md.c.f = uint16(op >> 32);
    edge.x.md.s.i = uint16(op >> 16);
    edge.x.md.s.f = uint16(op >>  0);
  };

  auto fetchShade = [&](u64 op) {
    op = fetch();
    shade.r.c.i = uint16(op >> 48);
    shade.g.c.i = uint16(op >> 32);
    shade.b.c.i = uint16(op >> 16);
    shade.a.c.i = uint16(op >>  0);
    op = fetch();
    shade.r.x.i = uint16(op >> 48);
    shade.g.x.i = uint16(op >> 32);
    shade.b.x.i = uint16(op >> 16);
    shade.a.x.i = uint16(op >>  0);
    op = fetch();
    shade.r.c.f = uint16(op >> 48);
    shade.g.c.f = uint16(op >> 32);
    shade.b.c.f = uint16(op >> 16);
    shade.a.c.f = uint16(op >>  0);
    op = fetch();
    shade.r.x.f = uint16(op >> 48);
    shade.g.x.f = uint16(op >> 32);
    shade.b.x.f = uint16(op >> 16);
    shade.a.x.f = uint16(op >>  0);
    op = fetch();
    shade.r.e.i = uint16(op >> 48);
    shade.g.e.i = uint16(op >> 32);
    shade.b.e.i = uint16(op >> 16);
    shade.a.e.i = uint16(op >>  0);
    op = fetch();
    shade.r.y.i = uint16(op >> 48);
    shade.g.y.i = uint16(op >> 32);
    shade.b.y.i = uint16(op >> 16);
    shade.a.y.i = uint16(op >>  0);
    op = fetch();
    shade.r.e.f = uint16(op >> 48);
    shade.g.e.f = uint16(op >> 32);
    shade.b.e.f = uint16(op >> 16);
    shade.a.e.f = uint16(op >>  0);
    op = fetch();
    shade.r.y.f = uint16(op >> 48);
    shade.g.y.f = uint16(op >> 32);
    shade.b.y.f = uint16(op >> 16);
    shade.a.y.f = uint16(op >>  0);
  };

  auto fetchTexture = [&](u64 op) {
    op = fetch();
    texture.s.c.i = uint16(op >> 48);
    texture.t.c.i = uint16(op >> 32);
    texture.w.c.i = uint16(op >> 16);
    op = fetch();
    texture.s.x.i = uint16(op >> 48);
    texture.t.x.i = uint16(op >> 32);
    texture.w.x.i = uint16(op >> 16);
    op = fetch();
    texture.s.c.f = uint16(op >> 48);
    texture.t.c.f = uint16(op >> 32);
    texture.w.c.f = uint16(op >> 16);
    op = fetch();
    texture.s.x.f = uint16(op >> 48);
    texture.t.x.f = uint16(op >> 32);
    texture.w.x.f = uint16(op >> 16);
    op = fetch();
    texture.s.e.i = uint16(op >> 48);
    texture.t.e.i = uint16(op >> 32);
    texture.w.e.i = uint16(op >> 16);
    op = fetch();
    texture.s.y.i = uint16(op >> 48);
    texture.t.y.i = uint16(op >> 32);
    texture.w.y.i = uint16(op >> 16);
    op = fetch();
    texture.s.e.f = uint16(op >> 48);
    texture.t.e.f = uint16(op >> 32);
    texture.w.e.f = uint16(op >> 16);
    op = fetch();
    texture.s.y.f = uint16(op >> 48);
    texture.t.y.f = uint16(op >> 32);
    texture.w.y.f = uint16(op >> 16);
  };

  auto fetchZBuffer = [&](u64 op) {
    op = fetch();
    zbuffer.d.i = uint16(op >> 48);
    zbuffer.d.f = uint16(op >> 32);
    zbuffer.x.i = uint16(op >> 16);
    zbuffer.x.f = uint16(op >>  0);
    op = fetch();
    zbuffer.e.i = uint16(op >> 48);
    zbuffer.e.f = uint16(op >> 32);
    zbuffer.y.i = uint16(op >> 16);
    zbuffer.y.f = uint16(op >>  0);
  };

  auto fetchRectangle = [&](u64 op) {
    rectangle.x.lo = uint12(op >> 44);
    rectangle.y.lo = uint12(op >> 32);
    rectangle.tile =  uint3(op >> 24);
    rectangle.x.hi = uint12(op >> 12);
    rectangle.y.hi = uint12(op >>  0);
    op = fetch();
    rectangle.s.i  = uint16(op >> 48);
    rectangle.t.i  = uint16(op >> 32);
    rectangle.s.f  = uint16(op >> 16);
    rectangle.t.f  = uint16(op >>  0);
  };

  while(command.current < command.end) {
    u64 op = fetch();

    if(debugger.tracer.command->enabled()) {
      debugger.command({hex(op, 16L), "  ", commandNames(op >> 56 & 0x3f, "Invalid")});
    }

    switch(op >> 56 & 0x3f) {

    case 0x00: {
      noOperation();
    } break;

    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07: {
      invalidOperation();
    } break;

    case 0x08: {
      fetchEdge(op);
      unshadedTriangle();
    } break;

    case 0x09: {
      fetchEdge(op);
      fetchZBuffer(op);
      unshadedZbufferTriangle();
    } break;

    case 0x0a: {
      fetchEdge(op);
      fetchTexture(op);
      textureTriangle();
    } break;

    case 0x0b: {
      fetchEdge(op);
      fetchTexture(op);
      fetchZBuffer(op);
      textureZbufferTriangle();
    } break;

    case 0x0c: {
      fetchEdge(op);
      fetchShade(op);
      shadedTriangle();
    } break;

    case 0x0d: {
      fetchEdge(op);
      fetchShade(op);
      fetchZBuffer(op);
      shadedZbufferTriangle();
    } break;

    case 0x0e: {
      fetchEdge(op);
      fetchShade(op);
      fetchTexture(op);
      shadedTextureTriangle();
    } break;

    case 0x0f: {
      fetchEdge(op);
      fetchShade(op);
      fetchTexture(op);
      fetchZBuffer(op);
      shadedTextureZbufferTriangle();
    } break;

    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23: {
      invalidOperation();
    } break;

    case 0x24: {
      fetchRectangle(op);
      textureRectangle();
    } break;

    case 0x25: {
      fetchRectangle(op);
      textureRectangleFlip();
    } break;

    case 0x26: {
      syncLoad();
    } break;

    case 0x27: {
      syncPipe();
    } break;

    case 0x28: {
      syncTile();
    } break;

    case 0x29: {
      syncFull();
    } break;

    case 0x2a: {
    //setKeyGB();
    } break;

    case 0x2b: {
    //setKeyR();
    } break;

    case 0x2c: {
    //setConvert();
    } break;

    case 0x2d: {
    //setScissor();
    } break;

    case 0x2e: {
    //setPrimitiveDepth();
    } break;

    case 0x2f: {
      other.atomicPrimitive = uint1(op >> 55);
      other.reserved1       = uint1(op >> 54);
      other.cycleType       = uint2(op >> 52);
      other.perspective     = uint1(op >> 51);
      other.detailTexture   = uint1(op >> 50);
      other.sharpenTexture  = uint1(op >> 49);
      other.lodTexture      = uint1(op >> 48);
      other.tlut            = uint1(op >> 47);
      other.tlutType        = uint1(op >> 46);
      other.sampleType      = uint1(op >> 45);
      other.midTexel        = uint1(op >> 44);
      other.bilerp[0]       = uint1(op >> 43);
      other.bilerp[1]       = uint1(op >> 42);
      other.convertOne      = uint1(op >> 41);
      other.colorKey        = uint1(op >> 40);
      other.colorDitherMode = uint2(op >> 38);
      other.alphaDitherMode = uint2(op >> 36);
      other.reserved2       = uint4(op >> 32);
      other.blend1a[0]      = uint2(op >> 30);
      other.blend1a[1]      = uint2(op >> 28);
      other.blend1b[0]      = uint2(op >> 26);
      other.blend1b[1]      = uint2(op >> 24);
      other.blend2a[0]      = uint2(op >> 22);
      other.blend2a[1]      = uint2(op >> 20);
      other.blend2b[0]      = uint2(op >> 18);
      other.blend2b[1]      = uint2(op >> 16);
      other.reserved3       = uint1(op >> 15);
      other.forceBlend      = uint1(op >> 14);
      other.alphaCoverage   = uint1(op >> 13);
      other.coverageXalpha  = uint1(op >> 12);
      other.zMode           = uint2(op >> 10);
      other.coverageMode    = uint2(op >>  8);
      other.colorOnCoverage = uint1(op >>  7);
      other.imageRead       = uint1(op >>  6);
      other.zUpdate         = uint1(op >>  5);
      other.zCompare        = uint1(op >>  4);
      other.antialias       = uint1(op >>  3);
      other.zSource         = uint1(op >>  2);
      other.ditherAlpha     = uint1(op >>  1);
      other.alphaCompare    = uint1(op >>  0);
      setOtherModes();
    } break;

    case 0x30: {
    //loadTextureLUT();
    } break;

    case 0x31: {
      invalidOperation();
    } break;

    case 0x32: {
    //setTileSize();
    } break;

    case 0x33: {
    //loadBlock();
    } break;

    case 0x34: {
    //loadTile();
    } break;

    case 0x35: {
    //setTile();
    } break;

    case 0x36: {
      auto xl = uint12(op >> 44);
      auto yl = uint12(op >> 32);
      auto xh = uint12(op >> 12);
      auto yh = uint12(op >>  0);
      fillRectangle(xl, yl, xh, yh);
    } break;

    case 0x37: {
      auto color = uint32(op >> 0);
      setFillColor(color);
    } break;

    case 0x38: {
    //setFogColor();
    } break;

    case 0x39: {
    //setBlendColor();
    } break;

    case 0x3a: {
    //setPrimitiveColor();
    } break;

    case 0x3b: {
    //setEnvironmentColor();
    } break;

    case 0x3c: {
      combine.sba.color[0] = uint4(op >> 52);
      combine.mul.color[0] = uint5(op >> 47);
      combine.sba.alpha[0] = uint3(op >> 44);
      combine.mul.alpha[0] = uint3(op >> 41);
      combine.sba.color[1] = uint4(op >> 37);
      combine.mul.color[1] = uint5(op >> 32);
      combine.sbb.color[0] = uint4(op >> 28);
      combine.sbb.color[1] = uint4(op >> 24);
      combine.sba.alpha[1] = uint3(op >> 21);
      combine.mul.alpha[1] = uint3(op >> 18);
      combine.add.color[0] = uint3(op >> 15);
      combine.sbb.alpha[0] = uint3(op >> 12);
      combine.add.alpha[0] = uint3(op >>  9);
      combine.add.color[1] = uint3(op >>  6);
      combine.sbb.alpha[1] = uint3(op >>  3);
      combine.add.alpha[1] = uint3(op >>  0);
      setCombineMode();
    } break;

    case 0x3d: {
      auto format      =  uint3(op >> 53);
      auto size        =  uint2(op >> 51);
      auto width       = uint10(op >> 32);
      auto dramAddress = uint26(op >>  0);
      setTextureImage(format, size, width, dramAddress);
    } break;

    case 0x3e: {
      auto dramAddress = uint26(op >>  0);
      setMaskImage(dramAddress);
    } break;

    case 0x3f: {
      auto format      =  uint3(op >> 53);
      auto size        =  uint2(op >> 51);
      auto width       = uint10(op >> 32);
      auto dramAddress = uint26(op >>  0);
      setColorImage(format, size, width, dramAddress);
    } break;

    }
  }
}

//0x00
auto RDP::noOperation() -> void {
}

//0x01-0x07; 0x10-0x23; 0x31
auto RDP::invalidOperation() -> void {
}

//0x08
auto RDP::unshadedTriangle() -> void {
}

//0x09
auto RDP::unshadedZbufferTriangle() -> void {
}

//0x0a
auto RDP::textureTriangle() -> void {
}

//0x0b
auto RDP::textureZbufferTriangle() -> void {
}

//0x0c
auto RDP::shadedTriangle() -> void {
}

//0x0d
auto RDP::shadedZbufferTriangle() -> void {
}

//0x0e
auto RDP::shadedTextureTriangle() -> void {
}

//0x0f
auto RDP::shadedTextureZbufferTriangle() -> void {
}

//0x24
auto RDP::textureRectangle() -> void {
}

//0x25
auto RDP::textureRectangleFlip() -> void {
}

//0x26
auto RDP::syncLoad() -> void {
}

//0x27
auto RDP::syncPipe() -> void {
}

//0x28
auto RDP::syncTile() -> void {
}

//0x29
auto RDP::syncFull() -> void {
  mi.raise(MI::IRQ::DP);
}

//0x2f
auto RDP::setOtherModes() -> void {
}

//0x36
auto RDP::fillRectangle(uint12 xl, uint12 yl, uint12 xh, uint12 yh) -> void {
  u32 depth = vi.io.colorDepth == 2 ? 2 : 4;
  u32 step  = vi.io.colorDepth == 2 ? 2 : 1;
  u32 shift = 2;

  xl >>= shift;
  yl >>= shift;
  xh >>= shift;
  yh >>= shift;

  u32 base = set.color.dramAddress;
  for(u32 y = yh; y <= yl; y++) {
    u32 line = base + y * vi.io.width * depth;
    u32 address = line + xh * depth;
    for(u32 x = xh; x <= xl; x += step) {
    //rdram.ram.writeWord(address, set.fill.color);
      address += 4;
    }
  }
}

//0x37
auto RDP::setFillColor(uint32 color) -> void {
  set.fill.color = color;
}

//0x3c
auto RDP::setCombineMode() -> void {
}

//0x3d
auto RDP::setTextureImage(uint3 format, uint2 size, uint10 width, uint26 dramAddress) -> void {
  set.texture.format = format;
  set.texture.size = size;
  set.texture.width = width;
  set.texture.dramAddress = dramAddress;
}

//0x3e
auto RDP::setMaskImage(uint26 dramAddress) -> void {
  set.mask.dramAddress = dramAddress;
}

//0x3f
auto RDP::setColorImage(uint3 format, uint2 size, uint10 width, uint26 dramAddress) -> void {
  set.color.format = format;
  set.color.size = size;
  set.color.width = width;
  set.color.dramAddress = dramAddress;
}
