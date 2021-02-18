//GPUREAD
auto GPU::readGP0() -> u32 {
  n32 data;

  if(io.mode == Mode::Status) {
    data = io.status;
    return data;
  }

  if(io.mode == Mode::CopyFromVRAM) {
    vram.mutex.lock();
    for(u32 loop : range(2)) {
      n10 x = io.copy.x + io.copy.px;
      n9  y = io.copy.y + io.copy.py;
      data = vram2D[y][x] << 16 | data >> 16;
      if(++io.copy.px >= io.copy.width) {
        io.copy.px = 0;
        if(++io.copy.py >= io.copy.height) {
          io.copy.py = 0;
          io.mode = Mode::Normal;
          break;
        }
      }
    }
    vram.mutex.unlock();
    return data;
  }

  data = io.status;
  return data;
}

auto GPU::writeGP0(u32 value, bool isThread) -> void {
  if(io.mode == Mode::CopyToVRAM) {
    vram.mutex.lock();
    for(u32 loop : range(2)) {
      n10 x = io.copy.x + io.copy.px;
      n9  y = io.copy.y + io.copy.py;
      u16 pixel = vram2D[y][x];
      if((pixel >> 15 & io.checkMaskBit) == 0) {
        vram2D[y][x] = value | io.forceMaskBit << 15;
      }
      value >>= 16;
      if(++io.copy.px >= io.copy.width) {
        io.copy.px = 0;
        if(++io.copy.py >= io.copy.height) {
          io.copy.py = 0;
          io.mode = Mode::Normal;
          break;
        }
      }
    }
    vram.mutex.unlock();
    return;
  }

  auto& queue = this->queue.gp0;

  n8  command = value >> 24;
  n24 data    = value >>  0;

  if(queue.empty()) {
    queue.command = command;
  //print("* GP0(", hex(command, 2L), ") = ", hex(value, 6L), "\n");
  } else {
    command = queue.command;
  //print("* GP0(", hex(command, 2L), ") = ", hex(value, 8L), " [", queue.length, "]\n");
  }

  Render render;
  render.command = command;
  render.dithering = io.dithering;
  render.semiTransparency = io.semiTransparency;
  render.checkMaskBit = io.checkMaskBit;
  render.forceMaskBit = io.forceMaskBit;
  render.drawingAreaOriginX1 = io.drawingAreaOriginX1;
  render.drawingAreaOriginY1 = io.drawingAreaOriginY1;
  render.drawingAreaOriginX2 = io.drawingAreaOriginX2;
  render.drawingAreaOriginY2 = io.drawingAreaOriginY2;
  render.drawingAreaOffsetX = io.drawingAreaOffsetX;
  render.drawingAreaOffsetY = io.drawingAreaOffsetY;
  render.textureDepth = io.textureDepth;
  render.texturePageBaseX = io.texturePageBaseX << 6;
  render.texturePageBaseY = io.texturePageBaseY << 8;
  render.texturePaletteX = io.texturePaletteX << 4;
  render.texturePaletteY = io.texturePaletteY << 0;
  render.texelMaskX = ~(io.textureWindowMaskX * 8);
  render.texelMaskY = ~(io.textureWindowMaskY * 8);
  render.texelOffsetX = (io.textureWindowOffsetX & io.textureWindowMaskX) * 8;
  render.texelOffsetY = (io.textureWindowOffsetY & io.textureWindowMaskY) * 8;

  auto setPalette = [&](u32 data) -> void {
    io.texturePaletteX = data >> 16 &  0x3f;
    io.texturePaletteY = data >> 22 & 0x1ff;

    render.texturePaletteX = io.texturePaletteX << 4;
    render.texturePaletteY = io.texturePaletteY << 0;
  };

  auto setPage = [&](u32 data) -> void {
    io.texturePageBaseX = data >> 16 & 15;
    io.texturePageBaseY = data >> 20 &  1;
    io.semiTransparency = data >> 21 &  3;
    io.textureDepth     = data >> 23 &  3;
    io.textureDisable   = data >> 27 &  1;

    render.texturePageBaseX = io.texturePageBaseX << 6;
    render.texturePageBaseY = io.texturePageBaseY << 8;
    render.semiTransparency = io.semiTransparency;
    render.textureDepth     = io.textureDepth;
  };

  //no operation
  if(command == 0x00) {
    return;
  }

  //clear cache
  if(command == 0x01) {
    return;
  }

  //fill rectangle
  if(command == 0x02) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //unknown
  if(command == 0x03) {
    debug(unimplemented, "GP0(03)");
    return;
  }

  //no operation
  if(command >= 0x04 && command <= 0x1e) {
    return;
  }

  //interrupt request
  if(command == 0x1f) {
    io.interrupt = 1;
    interrupt.raise(Interrupt::GPU);
    return;
  }

  //polygons:
  //  d0 = texture mode (0 = blend, 1 = raw)
  //  d1 = transparency
  //  d2 = texture mapping
  //  d3 = vertices (0 = triangle, 1 = quadrilateral)
  //  d4 = shading (0 = flat, 1 = gouraud)

  //monochrome triangle
  if(command == 0x20 || command == 0x21 || command == 0x22 || command == 0x23) {
    if(queue.write(value) < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]);
    renderer.queue(render);
    return queue.reset();
  }

  //textured triangle
  if(command == 0x24 || command == 0x25 || command == 0x26 || command == 0x27) {
    if(queue.write(value) < 7) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]).setTexel(queue.data[4]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[5]).setTexel(queue.data[6]);
    setPalette(queue.data[2]);
    setPage(queue.data[4]);
    renderer.queue(render);
    return queue.reset();
  }

  //monochrome quadrilateral
  if(command == 0x28 || command == 0x29 || command == 0x2a || command == 0x2b) {
    if(queue.write(value) < 5) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]);
    render.v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[4]);
    renderer.queue(render);
    return queue.reset();
  }

  //textured quadrilateral
  if(command == 0x2c || command == 0x2d || command == 0x2e || command == 0x2f) {
    if(queue.write(value) < 9) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]).setTexel(queue.data[4]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[5]).setTexel(queue.data[6]);
    render.v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    setPalette(queue.data[2]);
    setPage(queue.data[4]);
    renderer.queue(render);
    return queue.reset();
  }

  //shaded triangle
  if(command == 0x30 || command == 0x31 || command == 0x32 || command == 0x33) {
    if(queue.write(value) < 6) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    render.v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    renderer.queue(render);
    return queue.reset();
  }

  //shaded textured triangle
  if(command == 0x34 || command == 0x35 || command == 0x36 || command == 0x37) {
    if(queue.write(value) < 9) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[3]).setPoint(queue.data[4]).setTexel(queue.data[5]);
    render.v2 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    setPalette(queue.data[2]);
    setPage(queue.data[5]);
    renderer.queue(render);
    return queue.reset();
  }

  //shaded quadrilateral
  if(command == 0x38 || command == 0x39 || command == 0x3a || command == 0x3b) {
    if(queue.write(value) < 8) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    render.v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    render.v3 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]);
    renderer.queue(render);
    return queue.reset();
  }

  //shaded textured quadrilateral
  if(command == 0x3c || command == 0x3d || command == 0x3e || command == 0x3f) {
    if(queue.write(value) < 12) return;
    render.v0 = Vertex().setColor(queue.data[ 0]).setPoint(queue.data[ 1]).setTexel(queue.data[ 2]);
    render.v1 = Vertex().setColor(queue.data[ 3]).setPoint(queue.data[ 4]).setTexel(queue.data[ 5]);
    render.v2 = Vertex().setColor(queue.data[ 6]).setPoint(queue.data[ 7]).setTexel(queue.data[ 8]);
    render.v3 = Vertex().setColor(queue.data[ 9]).setPoint(queue.data[10]).setTexel(queue.data[11]);
    setPalette(queue.data[2]);
    setPage(queue.data[5]);
    renderer.queue(render);
    return queue.reset();
  }

  //lines:
  //  d1 = transparency
  //  d3 = mode (0 = single, 1 = poly)
  //  d4 = shading (0 = flat, 1 = gouraud)

  //monochrome line
  if(command == 0x40 || command == 0x41 || command == 0x42 || command == 0x43
  || command == 0x44 || command == 0x45 || command == 0x46 || command == 0x47) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //monochrome poly-line
  if(command == 0x48 || command == 0x49 || command == 0x4a || command == 0x4b
  || command == 0x4c || command == 0x4d || command == 0x4e || command == 0x4f) {
    if((value & 0xf000f000) != 0x50005000) return (void)queue.write(value);
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    for(u32 n = 2; n < queue.length; n += 1) {
      render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[n]);
      renderer.queue(render);
      render.v0 = render.v1;
    }
    return queue.reset();
  }

  //shaded line
  if(command == 0x50 || command == 0x51 || command == 0x52 || command == 0x53
  || command == 0x54 || command == 0x55 || command == 0x56 || command == 0x57) {
    if(queue.write(value) < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    renderer.queue(render);
    return queue.reset();
  }

  //shaded poly-line
  if(command == 0x58 || command == 0x59 || command == 0x5a || command == 0x5b
  || command == 0x5c || command == 0x5d || command == 0x5e || command == 0x5f) {
    if((value & 0xf000f000) != 0x50005000) return (void)queue.write(value);
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    for(u32 n = 2; n + 1 < queue.length; n += 2) {
      render.v1 = Vertex().setColor(queue.data[n + 0]).setPoint(queue.data[n + 1]);
      renderer.queue(render);
      render.v0 = render.v1;
    }
    return queue.reset();
  }

  //rectangles:
  //  d0 = texture mode (0 = blend, 1 = raw)
  //  d1 = transparency
  //  d2 = texture mapping
  //  d3-d4 = size (0 = variable, 1 = 1x1, 2 = 8x8, 3 = 16x16)

  //monochrome rectangle (variable size)
  if(command == 0x60 || command == 0x61 || command == 0x62 || command == 0x63) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //textured rectangle (variable size)
  if(command == 0x64 || command == 0x65 || command == 0x66 || command == 0x67) {
    if(queue.write(value) < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(queue.data[3]);
    setPalette(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //monochrome rectangle (1x1 size)
  if(command == 0x68 || command == 0x69 || command == 0x6a || command == 0x6b) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(1, 1);
    renderer.queue(render);
    return queue.reset();
  }

  //textured rectangle (1x1 size)
  if(command == 0x6c || command == 0x6d || command == 0x6e || command == 0x6f) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(1, 1);
    setPalette(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //monochrome rectangle (8x8 size)
  if(command == 0x70 || command == 0x71 || command == 0x72 || command == 0x73) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(8, 8);
    renderer.queue(render);
    return queue.reset();
  }

  //textured rectangle (8x8 size)
  if(command == 0x74 || command == 0x75 || command == 0x76 || command == 0x77) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(8, 8);
    setPalette(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //monochrome rectangle (16x16 size)
  if(command == 0x78 || command == 0x79 || command == 0x7a || command == 0x7b) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(16, 16);
    renderer.queue(render);
    return queue.reset();
  }

  //textured rectangle (16x16 size)
  if(command == 0x7c || command == 0x7d || command == 0x7e || command == 0x7f) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(16, 16);
    setPalette(queue.data[2]);
    renderer.queue(render);
    return queue.reset();
  }

  //copy rectangle (VRAM to VRAM)
  if(command >= 0x80 && command <= 0x9f) {
    if(queue.write(value) < 4) return;
    u16 sourceX = queue.data[1].bit( 0,15);
    u16 sourceY = queue.data[1].bit(16,31);
    u16 targetX = queue.data[2].bit( 0,15);
    u16 targetY = queue.data[2].bit(16,31);
    u16 width   = queue.data[3].bit( 0,15);
    u16 height  = queue.data[3].bit(16,31);
    for(u32 y : range(height)) {
      for(u32 x : range(width)) {
        u16 pixel = vram2D[n9(y + sourceY)][n10(x + sourceX)];
        if((pixel >> 15 & io.checkMaskBit) == 0) {
          vram2D[n9(y + targetY)][n10(x + targetX)] = pixel | io.forceMaskBit << 15;
        }
      }
    }
    return queue.reset();
  }

  //copy rectangle (CPU to VRAM)
  if(command >= 0xa0 && command <= 0xbf) {
    if(queue.write(value) < 3) return;
    io.copy.x      = (queue.data[1].bit( 0,15) & 1023);
    io.copy.y      = (queue.data[1].bit(16,31) &  511);
    io.copy.width  = (queue.data[2].bit( 0,15) - 1 & 1023) + 1;
    io.copy.height = (queue.data[2].bit(16,31) - 1 &  511) + 1;
    io.copy.px     = 0;
    io.copy.py     = 0;
    io.mode        = Mode::CopyToVRAM;
    return queue.reset();
  }

  //copy rectangle (VRAM to CPU)
  if(command >= 0xc0 && command <= 0xdf) {
    if(queue.write(value) < 3) return;
    io.copy.x      = (queue.data[1].bit( 0,15) & 1023);
    io.copy.y      = (queue.data[1].bit(16,31) &  511);
    io.copy.width  = (queue.data[2].bit( 0,15) - 1 & 1023) + 1;
    io.copy.height = (queue.data[2].bit(16,31) - 1 &  511) + 1;
    io.copy.px     = 0;
    io.copy.py     = 0;
    io.mode        = Mode::CopyFromVRAM;
    return queue.reset();
  }

  //no operation
  if(command == 0xe0) {
    return;
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

  //no operation
  if(command >= 0xe7 && command <= 0xef) {
    return;
  }

  //unknown
  if(command >= 0xf0 && command <= 0xff) {
    debug(unimplemented, "GP0(", hex(command, 2L), ") = ", hex(value, 6L));
  }
}
