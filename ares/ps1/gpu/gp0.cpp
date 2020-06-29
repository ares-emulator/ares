auto GPU::gp0(u32 value) -> void {
  auto& queue = this->queue.gp0;

   uint8 command = value >> 24;
  uint24 data    = value >>  0;

  if(queue.empty()) {
    queue.command = command;
  //print("* GP0(", hex(command, 2L), ") = ", hex(value, 6L), "\n");
  } else {
    command = queue.command;
  //print("* GP0(", hex(command, 2L), ") = ", hex(value, 8L), " [", queue.length, "]\n");
  }

  //unknown
  if(command == 0x00) {
    return;
  }

  //clear cache
  if(command == 0x01) {
    return;
  }

  //monochrome triangle
  if(command == 0x20 || command == 0x21 || command == 0x22 || command == 0x23) {
    if(queue.write(value) < 4) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    auto v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    auto v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]);
    auto ts = Texture();
    if(command == 0x20 || command == 0x21) renderTriangle<Render::Color>(v0, v1, v2, ts);
    if(command == 0x22 || command == 0x23) renderTriangle<Render::Color | Render::Alpha>(v0, v1, v2, ts);
    return queue.reset();
  }

  //textured triangle
  if(command == 0x24 || command == 0x25 || command == 0x26 || command == 0x27) {
    if(queue.write(value) < 7) return;
    auto v0 = Vertex().setColor(0).setPoint(queue.data[1]).setTexel(queue.data[2]);
    auto v1 = Vertex().setColor(0).setPoint(queue.data[3]).setTexel(queue.data[4]);
    auto v2 = Vertex().setColor(0).setPoint(queue.data[5]).setTexel(queue.data[6]);
    auto ts = Texture().setPalette(queue.data[2]).setPage(queue.data[4]);
    if(command == 0x24) renderTriangle<Render::Texel | Render::ModulateColor>(v0, v1, v2, ts);
    if(command == 0x25) renderTriangle<Render::Texel>(v0, v1, v2, ts);
    if(command == 0x26) renderTriangle<Render::Texel | Render::Alpha | Render::ModulateColor>(v0, v1, v2, ts);
    if(command == 0x27) renderTriangle<Render::Texel | Render::Alpha>(v0, v1, v2, ts);
    return queue.reset();
  }

  //monochrome quadrilateral
  if(command == 0x28 || command == 0x29 || command == 0x2a || command == 0x2b) {
    if(queue.write(value) < 5) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    auto v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    auto v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]);
    auto v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[4]);
    auto ts = Texture();
    if(command == 0x28 || command == 0x29) renderQuadrilateral<Render::Color>(v0, v1, v2, v3, ts);
    if(command == 0x2a || command == 0x2b) renderQuadrilateral<Render::Color | Render::Alpha>(v0, v1, v2, v3, ts);
    return queue.reset();
  }

  //textured quadrilateral
  if(command == 0x2c || command == 0x2d || command == 0x2e || command == 0x2f) {
    if(queue.write(value) < 9) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    auto v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]).setTexel(queue.data[4]);
    auto v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[5]).setTexel(queue.data[6]);
    auto v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    auto ts = Texture().setPalette(queue.data[2]).setPage(queue.data[4]);
    if(command == 0x2c) renderQuadrilateral<Render::Texel | Render::ModulateColor>(v0, v1, v2, v3, ts);
    if(command == 0x2d) renderQuadrilateral<Render::Texel>(v0, v1, v2, v3, ts);
    if(command == 0x2e) renderQuadrilateral<Render::Texel | Render::Alpha | Render::ModulateColor>(v0, v1, v2, v3, ts);
    if(command == 0x2f) renderQuadrilateral<Render::Texel | Render::Alpha>(v0, v1, v2, v3, ts);
    return queue.reset();
  }

  //shaded triangle
  if(command == 0x30 || command == 0x31 || command == 0x32 || command == 0x33) {
    if(queue.write(value) < 6) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    auto v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    auto v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    auto ts = Texture();
    if(command == 0x30 || command == 0x31) renderTriangle<Render::Shade>(v0, v1, v2, ts);
    if(command == 0x32 || command == 0x33) renderTriangle<Render::Shade | Render::Alpha>(v0, v1, v2, ts);
    return queue.reset();
  }

  //shaded textured triangle
  if(command == 0x34 || command == 0x35 || command == 0x36 || command == 0x37) {
    if(queue.write(value) < 9) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    auto v1 = Vertex().setColor(queue.data[3]).setPoint(queue.data[4]).setTexel(queue.data[5]);
    auto v2 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    auto ts = Texture().setPalette(queue.data[2]).setPage(queue.data[5]);
    if(command == 0x34 || command == 0x35) renderTriangle<Render::Texel | Render::ModulateShade>(v0, v1, v2, ts);
    if(command == 0x36 || command == 0x37) renderTriangle<Render::Texel | Render::Alpha | Render::ModulateShade>(v0, v1, v2, ts);
    return queue.reset();
  }

  //shaded quadrilateral
  if(command == 0x38 || command == 0x39 || command == 0x3a || command == 0x3b) {
    if(queue.write(value) < 8) return;
    auto v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    auto v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    auto v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    auto v3 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]);
    auto ts = Texture();
    if(command == 0x38 || command == 0x39) renderQuadrilateral<Render::Shade>(v0, v1, v2, v3, ts);
    if(command == 0x3a || command == 0x3b) renderQuadrilateral<Render::Shade | Render::Alpha>(v0, v1, v2, v3, ts);
    return queue.reset();
  }

  //shaded textured quadrilateral
  if(command == 0x3c || command == 0x3d || command == 0x3e || command == 0x3f) {
    if(queue.write(value) < 12) return;
    auto v0 = Vertex().setColor(queue.data[ 0]).setPoint(queue.data[ 1]).setTexel(queue.data[ 2]);
    auto v1 = Vertex().setColor(queue.data[ 3]).setPoint(queue.data[ 4]).setTexel(queue.data[ 5]);
    auto v2 = Vertex().setColor(queue.data[ 6]).setPoint(queue.data[ 7]).setTexel(queue.data[ 8]);
    auto v3 = Vertex().setColor(queue.data[ 9]).setPoint(queue.data[10]).setTexel(queue.data[11]);
    auto ts = Texture().setPalette(queue.data[2]).setPage(queue.data[5]);
    if(command == 0x3c || command == 0x3d) renderQuadrilateral<Render::Texel | Render::ModulateShade>(v0, v1, v2, v3, ts);
    if(command == 0x3e || command == 0x3f) renderQuadrilateral<Render::Texel | Render::Alpha | Render::ModulateShade>(v0, v1, v2, v3, ts);
    return queue.reset();
  }

  //copy rectangle (CPU to VRAM)
  if(command == 0xa0) {
    if(queue.length < 3) return (void)queue.write(value);
    uint16 x       = (queue.data[1].bit( 0,15) & 1023);
    uint16 y       = (queue.data[1].bit(16,31) &  511);
    uint16 width   = (queue.data[2].bit( 0,15) - 1 & 1023) + 1;
    uint16 height  = (queue.data[2].bit(16,31) - 1 &  511) + 1;
    for(uint pixel : range(2)) {
      uint10 offsetX = x + queue.counterX;
       uint9 offsetY = y + queue.counterY;
      vram.writeHalf((offsetY * 1024 + offsetX) * 2, value);
      value >>= 16;
      if(++queue.counterX >= width) {
        queue.counterX = 0;
        if(++queue.counterY >= height) {
          queue.counterY = 0;
          return queue.reset();
        }
      }
    }
    return;
  }

  //copy rectangle (VRAM to CPU)
  if(command == 0xc0) {
    if(queue.length < 3) return (void)queue.write(value);
    uint16 x      = (queue.data[1].bit( 0,15) & 1023);
    uint16 y      = (queue.data[1].bit(16,31) &  511);
    uint16 width  = (queue.data[2].bit( 0,15) - 1 & 1023) + 1;
    uint16 height = (queue.data[2].bit(16,31) - 1 &  511) + 1;
    return queue.reset();
  }

  //draw mode
  if(command == 0xe1) {
    io.texturePageBaseX = data.bit( 0, 3);
    io.texturePageBaseY = data.bit( 4);
    io.semiTransparency = data.bit( 5, 6);
    io.textureDepth     = data.bit( 7, 8);
    io.dithering        = data.bit( 9);
    io.drawToDisplay    = data.bit(10);
    io.textureDisable   = data.bit(11);
    io.textureFlipX     = data.bit(12);
    io.textureFlipY     = data.bit(13);
    return;
  }

  //texture window
  if(command == 0xe2) {
    io.textureWindowMaskX   = data.bit( 0, 4);
    io.textureWindowMaskY   = data.bit( 5, 9);
    io.textureWindowOffsetX = data.bit(10,14);
    io.textureWindowOffsetY = data.bit(15,19);
    return;
  }

  //set drawing area (top left)
  if(command == 0xe3) {
    io.drawingAreaOriginX1 = data.bit( 0, 9);
    io.drawingAreaOriginY1 = data.bit(10,19);
    return;
  }

  //set drawing area (bottom right)
  if(command == 0xe4) {
    io.drawingAreaOriginX2 = data.bit( 0, 9);
    io.drawingAreaOriginY2 = data.bit(10,19);
    return;
  }

  //set drawing area
  if(command == 0xe5) {
    io.drawingAreaOffsetX = data.bit( 0,10);
    io.drawingAreaOffsetY = data.bit(11,21);
    return;
  }

  //mask bit
  if(command == 0xe6) {
    io.forceMaskBit = data.bit(0);
    io.checkMaskBit = data.bit(1);
    return;
  }

//print("* GP0(", hex(command, 2L), ") = ", hex(value, 6L), "\n");
}
