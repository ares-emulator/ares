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

  //monochrome quadrilateral
  if(command == 0x28) {
    if(queue.write(value) < 5) return;
    Color c;
    c.r = queue.data[0].bit( 0, 7);
    c.g = queue.data[0].bit( 8,15);
    c.b = queue.data[0].bit(16,23);
    Point p0;
    p0.x = queue.data[1].bit( 0,15);
    p0.y = queue.data[1].bit(16,31);
    Point p1;
    p1.x = queue.data[2].bit( 0,15);
    p1.y = queue.data[2].bit(16,31);
    Point p2;
    p2.x = queue.data[3].bit( 0,15);
    p2.y = queue.data[3].bit(16,31);
    Point p3;
    p3.x = queue.data[4].bit( 0,15);
    p3.y = queue.data[4].bit(16,31);
    renderSolidOpaqueQuadrilateral(p0, p1, p2, p3, c);
    return queue.reset();
  }

  //textured quadrilateral with blending
  if(command == 0x2c) {
    if(queue.write(value) < 9) return;
    return queue.reset();
  }

  //shaded triangle
  if(command == 0x30) {
    if(queue.write(value) < 6) return;
    return queue.reset();
  }

  //shaded quadrilateral
  if(command == 0x38) {
    if(queue.write(value) < 8) return;
    Vertex v0;
    v0.r = queue.data[0].bit( 0, 7);
    v0.g = queue.data[0].bit( 8,15);
    v0.b = queue.data[0].bit(16,23);
    v0.x = queue.data[1].bit( 0,15);
    v0.y = queue.data[1].bit(16,31);
    Vertex v1;
    v1.r = queue.data[2].bit( 0, 7);
    v1.g = queue.data[2].bit( 8,15);
    v1.b = queue.data[2].bit(16,23);
    v1.x = queue.data[3].bit( 0,15);
    v1.y = queue.data[3].bit(16,31);
    Vertex v2;
    v2.r = queue.data[4].bit( 0, 7);
    v2.g = queue.data[4].bit( 8,15);
    v2.b = queue.data[4].bit(16,23);
    v2.x = queue.data[5].bit( 0,15);
    v2.y = queue.data[5].bit(16,31);
    Vertex v3;
    v3.r = queue.data[6].bit( 0, 7);
    v3.g = queue.data[6].bit( 8,15);
    v3.b = queue.data[6].bit(16,23);
    v3.x = queue.data[7].bit( 0,15);
    v3.y = queue.data[7].bit(16,31);
    renderShadedOpaqueQuadrilateral(v0, v1, v2, v3);
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
