auto RDP::render() -> void {
  auto& memory = !command.source ? rdram.ram : rsp.dmem;
  auto fetch = [&]() -> u64 {
    u64 op = memory.readDouble(command.current);
    command.current += 8;
    return op;
  };

  command.current = command.start;
  while(command.current < command.end) {
    u64 op = fetch();

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
      command.current += 8 * 3;  //edge
    //unshadedTriangle();
    } break;

    case 0x09: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 2;  //zbuffer
    //unshadedZbufferTriangle();
    } break;

    case 0x0a: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //texture
    //textureTriangle();
    } break;

    case 0x0b: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //texture
      command.current += 8 * 2;  //zbuffer
    //textureZbufferTriangle();
    } break;

    case 0x0c: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //shade
    //shadedTriangle();
    } break;

    case 0x0d: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //shade
      command.current += 8 * 2;  //zbuffer
    //shadedZbufferTriangle();
    } break;

    case 0x0e: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //shade
      command.current += 8 * 8;  //texture
    //shadedTextureTriangle();
    } break;

    case 0x0f: {
      command.current += 8 * 3;  //edge
      command.current += 8 * 8;  //shade
      command.current += 8 * 8;  //texture
      command.current += 8 * 2;  //zbuffer
    //shadedTextureZbufferTriangle();
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
      auto xl   = uint12(op >> 44);
      auto yl   = uint12(op >> 32);
      auto tile =  uint3(op >> 24);
      auto xh   = uint12(op >> 12);
      auto yh   = uint12(op >>  0);
      op = fetch();
      auto s    = uint16(op >> 48);
      auto t    = uint16(op >> 32);
      auto dsdx = uint16(op >> 16);
      auto dsdy = uint16(op >>  0);
      textureRectangle(xl, yl, tile, xh, yh, s, t, dsdx, dsdy);
    } break;

    case 0x25: {
      auto xl   = uint12(op >> 44);
      auto yl   = uint12(op >> 32);
      auto tile =  uint3(op >> 24);
      auto xh   = uint12(op >> 12);
      auto yh   = uint12(op >>  0);
      op = fetch();
      auto s    = uint16(op >> 48);
      auto t    = uint16(op >> 32);
      auto dsdx = uint16(op >> 16);
      auto dsdy = uint16(op >>  0);
      textureRectangleFlip(xl, yl, tile, xh, yh, s, t, dsdx, dsdy);
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
    //setOtherModes();
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
    //setCombineMode();
    } break;

    case 0x3d: {
      auto format      =  uint3(op >> 53);
      auto size        =  uint2(op >> 51);
      auto width       = uint10(op >> 32);
      auto dramAddress = uint26(op >>  0);
      setTextureImage(format, size, width, dramAddress);
    } break;

    case 0x3e: {
    //setMaskImage();
    } break;

    case 0x3f: {
    //setColorImage();
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

//0x24
auto RDP::textureRectangle(
  uint12 xl, uint12 yl, uint3 tile, uint12 xh, uint12 yh,
  uint16 s, uint16 t, uint16 dsdx, uint16 dsdy
) -> void {
}

//0x25
auto RDP::textureRectangleFlip(
  uint12 xl, uint12 yl, uint3 tile, uint12 xh, uint12 yh,
  uint16 s, uint16 t, uint16 dsdx, uint16 dsdy
) -> void {
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

//0x36
auto RDP::fillRectangle(uint12 xl, uint12 yl, uint12 xh, uint12 yh) -> void {
  u32 depth = vi.io.colorDepth == 2 ? 2 : 4;
  u32 step  = vi.io.colorDepth == 2 ? 2 : 1;
  u32 shift = 2;

  xl >>= shift;
  yl >>= shift;
  xh >>= shift;
  yh >>= shift;

  u32 base = vi.io.dramAddress;
  for(u32 y = yh; y <= yl; y++) {
    u32 line = base + y * vi.io.width * depth;
    u32 address = line + xh * depth;
    for(u32 x = xh; x <= xl; x += step) {
      rdram.ram.writeWord(address, set.fill.color);
      address += 4;
    }
  }
}

//0x37
auto RDP::setFillColor(uint32 color) -> void {
  set.fill.color = color;
}

//0x3d
auto RDP::setTextureImage(uint3 format, uint2 size, uint10 width, uint26 dramAddress) -> void {
  set.texture.format = format;
  set.texture.size = size;
  set.texture.width = width;
  set.texture.dramAddress = dramAddress;
}
