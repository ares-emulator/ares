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
      key.g.width  = uint12(op >> 44);
      key.b.width  = uint12(op >> 32);
      key.g.center =  uint8(op >> 24);
      key.g.scale  =  uint8(op >> 16);
      key.b.center =  uint8(op >>  8);
      key.b.scale  =  uint8(op >>  0);
      setKeyGB();
    } break;

    case 0x2b: {
      key.r.width  = uint12(op >> 16);
      key.r.center =  uint8(op >>  8);
      key.r.scale  =  uint8(op >>  0);
      setKeyR();
    } break;

    case 0x2c: {
      convert.k[0] = uint9(op >> 45);
      convert.k[1] = uint9(op >> 36);
      convert.k[2] = uint9(op >> 27);
      convert.k[3] = uint9(op >> 18);
      convert.k[4] = uint9(op >>  9);
      convert.k[5] = uint8(op >>  0);
      setConvert();
    } break;

    case 0x2d: {
      scissor.x.hi  = uint12(op >> 44);
      scissor.y.hi  = uint12(op >> 32);
      scissor.field =  uint1(op >> 25);
      scissor.odd   =  uint1(op >> 24);
      scissor.x.lo  = uint12(op >> 12);
      scissor.y.lo  = uint12(op >>  0);
      setScissor();
    } break;

    case 0x2e: {
      primitiveDepth.z      = uint16(op >> 16);
      primitiveDepth.deltaZ = uint16(op >>  0);
      setPrimitiveDepth();
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
      tlut.s.lo  = uint12(op >> 44);
      tlut.t.lo  = uint12(op >> 32);
      tlut.index =  uint3(op >> 24);
      tlut.s.hi  = uint12(op >> 12);
      tlut.t.hi  = uint12(op >>  0);
      loadTLUT();
    } break;

    case 0x31: {
      invalidOperation();
    } break;

    case 0x32: {
      tileSize.s.lo  = uint12(op >> 44);
      tileSize.t.lo  = uint12(op >> 32);
      tileSize.index =  uint3(op >> 24);
      tileSize.s.hi  = uint12(op >> 12);
      tileSize.t.hi  = uint12(op >>  0);
      setTileSize();
    } break;

    case 0x33: {
      load_.block.s.lo  = uint12(op >> 44);
      load_.block.t.lo  = uint12(op >> 32);
      load_.block.index =  uint3(op >> 24);
      load_.block.s.hi  = uint12(op >> 12);
      load_.block.t.hi  = uint12(op >>  0);
      loadBlock();
    } break;

    case 0x34: {
      load_.tile.s.lo  = uint12(op >> 44);
      load_.tile.t.lo  = uint12(op >> 32);
      load_.tile.index =  uint3(op >> 24);
      load_.tile.s.hi  = uint12(op >> 12);
      load_.tile.t.hi  = uint12(op >>  0);
      loadTile();
    } break;

    case 0x35: {
      tile.format   = uint3(op >> 53);
      tile.size     = uint2(op >> 51);
      tile.line     = uint9(op >> 41);
      tile.address  = uint9(op >> 32);
      tile.index    = uint3(op >> 24);
      tile.palette  = uint4(op >> 20);
      tile.t.clamp  = uint1(op >> 19);
      tile.t.mirror = uint1(op >> 18);
      tile.t.mask   = uint4(op >> 14);
      tile.t.shift  = uint4(op >> 10);
      tile.s.clamp  = uint1(op >>  9);
      tile.s.mirror = uint1(op >>  8);
      tile.s.mask   = uint4(op >>  4);
      tile.s.shift  = uint4(op >>  0);
      setTile();
    } break;

    case 0x36: {
      fillRectangle_.x.lo = uint12(op >> 44);
      fillRectangle_.y.lo = uint12(op >> 32);
      fillRectangle_.x.hi = uint12(op >> 12);
      fillRectangle_.y.hi = uint12(op >>  0);
      fillRectangle();
    } break;

    case 0x37: {
      set.fill.color = uint32(op >> 0);
      setFillColor();
    } break;

    case 0x38: {
      fog.red   = uint8(op >> 24);
      fog.green = uint8(op >> 16);
      fog.blue  = uint8(op >>  8);
      fog.alpha = uint8(op >>  0);
      setFogColor();
    } break;

    case 0x39: {
      blend.red   = uint8(op >> 24);
      blend.green = uint8(op >> 16);
      blend.blue  = uint8(op >>  8);
      blend.alpha = uint8(op >>  0);
      setBlendColor();
    } break;

    case 0x3a: {
      primitive.minimum  = uint4(op >> 40);
      primitive.fraction = uint8(op >> 32);
      primitive.red      = uint8(op >> 24);
      primitive.green    = uint8(op >> 16);
      primitive.blue     = uint8(op >>  8);
      primitive.alpha    = uint8(op >>  0);
      setPrimitiveColor();
    } break;

    case 0x3b: {
      environment.red   = uint8(op >> 24);
      environment.green = uint8(op >> 16);
      environment.blue  = uint8(op >>  8);
      environment.alpha = uint8(op >>  0);
      setEnvironmentColor();
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
      set.texture.format      =  uint3(op >> 53);
      set.texture.size        =  uint2(op >> 51);
      set.texture.width       = uint10(op >> 32);
      set.texture.dramAddress = uint26(op >>  0);
      setTextureImage();
    } break;

    case 0x3e: {
      set.mask.dramAddress = uint26(op >>  0);
      setMaskImage();
    } break;

    case 0x3f: {
      set.color.format      =  uint3(op >> 53);
      set.color.size        =  uint2(op >> 51);
      set.color.width       = uint10(op >> 32);
      set.color.dramAddress = uint26(op >>  0);
      setColorImage();
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

//0x2a
auto RDP::setKeyGB() -> void {
}

//0x2b
auto RDP::setKeyR() -> void {
}

//0x2c
auto RDP::setConvert() -> void {
}

//0x2d
auto RDP::setScissor() -> void {
}

//0x2e
auto RDP::setPrimitiveDepth() -> void {
}

//0x2f
auto RDP::setOtherModes() -> void {
}

//0x30
auto RDP::loadTLUT() -> void {
}

//0x32
auto RDP::setTileSize() -> void {
}

//0x33
auto RDP::loadBlock() -> void {
}

//0x34
auto RDP::loadTile() -> void {
}

//0x35
auto RDP::setTile() -> void {
}

//0x36
auto RDP::fillRectangle() -> void {
}

//0x37
auto RDP::setFillColor() -> void {
}

//0x38
auto RDP::setFogColor() -> void {
}

//0x39
auto RDP::setBlendColor() -> void {
}

//0x3a
auto RDP::setPrimitiveColor() -> void {
}

//0x3b
auto RDP::setEnvironmentColor() -> void {
}

//0x3c
auto RDP::setCombineMode() -> void {
}

//0x3d
auto RDP::setTextureImage() -> void {
}

//0x3e
auto RDP::setMaskImage() -> void {
}

//0x3f
auto RDP::setColorImage() -> void {
}
