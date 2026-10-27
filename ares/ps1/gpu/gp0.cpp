//GPUREAD
auto GPU::readGP0() -> u32 {
  n32 data;

  if(io.mode == Mode::CopyFromVRAM) {
    renderer.synchronize();
    vram.mutex.lock();

    auto isOdd = (io.copy.width * io.copy.height) & 1;
    auto lastLine = io.copy.py == io.copy.height - 1;

    for(u32 loop : range(2)) {
      n10 x = io.copy.x + io.copy.px;
      n9  y = io.copy.y + io.copy.py;
      u16 pixel = vram2D[y & 511][x & 1023];
      data |= (loop == 0) ? pixel : pixel << 16;
      if(++io.copy.px >= io.copy.width + (isOdd && lastLine ? 1 : 0)) {
        io.copy.px = 0;
        if(++io.copy.py >= io.copy.height) {
          io.copy.py = 0;
          io.mode = Mode::Normal;
          break;
        }
      }
    }
    vram.mutex.unlock();
    io.vramReadWord = data;
    io.vramReadWordValid = 1;

    if(!io.vramReadLatchValid && io.mode == Mode::CopyFromVRAM) {
      io.dataReadBuffer = data;
      io.vramReadLatchValid = 1;
    }
    if(io.mode == Mode::Normal) {
      u32 length = io.readTransferFifoLength;
      io.readTransferFifoLength = 0;
      for(u32 index : range(length)) executeGP0(io.readTransferFifo[index]);
      if(!fifo.empty()) stepFifo();
    }
    return data;
  }

  data = io.dataReadBuffer;
  return data;
}

static constexpr auto commandUsesFifo(u8 command) -> bool {
  if(command == 0x01 || command == 0x02 || command == 0x1f) return true;
  if(command >= 0x20 && command <= 0xdf) return true;
  return command == 0xe1 || command == 0xe2 || command == 0xe6;
}

static constexpr auto commandWords(u8 cmd) -> u32 {
  if(cmd == 0x02) return 3;

  if(cmd >= 0x20 && cmd <= 0x3f) {
    bool gouraud = cmd & 0x10;
    bool quad = cmd & 0x08;
    bool textured = cmd & 0x04;
    u32 vertices = quad ? 4 : 3;
    return gouraud ? (vertices * (textured ? 3 : 2))
                   : (1 + vertices * (textured ? 2 : 1));
  }

  if(cmd >= 0x40 && cmd <= 0x5f) {
    return cmd & 0x10 ? 4 : 3;
  }

  if(cmd >= 0x60 && cmd <= 0x7f) {
    u8 size = cmd >> 3 & 3;
    bool textured = cmd & 0x04;
    if(size == 0) return textured ? 4 : 3;
    return textured ? 3 : 2;
  }

  if(cmd >= 0x80 && cmd <= 0x9f) return 4;
  if(cmd >= 0xa0 && cmd <= 0xdf) return 3;

  return 1;
}
static constexpr auto commandInfo = [] {
  std::array<u8, 256> result{};
  for(u32 command = 0; command < result.size(); command++)
    result[command] = commandWords(command) | (commandUsesFifo(command) ? 0x10 : 0);
  return result;
}();

static auto occupiesFifo(u8 command) -> bool { return commandInfo[command] & 0x10; }
auto GPU::commandLength(u32 word) -> u32 { return commandInfo[word >> 24] & 0x0f; }

auto GPU::stepFifo() -> void {
  if((!drawBusy() || !fill.active) && latch.ready) executeLatch();
  while((!drawBusy() || !fill.active) && !fifo.empty()) {
    if(io.mode == Mode::CopyFromVRAM) break;
    u32 word = fifo.read();
    executeGP0(word);
  }
}

auto GPU::finishPendingRender() -> void {
  if(!pendingTextureTiming) return;
  renderer.synchronize();
  io.pcounter += renderer.textureCycles - textureCyclesAtQueue;
  io.pcounter += (u32)((renderer.textureBlendPixels - textureBlendPixelsAtQueue) * (1.298 - 0.858));
  pendingTextureTiming = false;
}

auto GPU::executeLatch() -> void {
  u32 length = latch.length;
  latch.ready = 0;
  latch.length = 0;
  latch.target = 0;
  for(u32 index : range(length)) executeGP0(latch.data[index]);
}

auto GPU::writeGP0(u32 value, bool isThread, bool isDMA) -> void {
  if(io.mode == Mode::Normal && !fill.active && (latch.ready || !fifo.empty())) stepFifo();
  if(io.mode == Mode::CopyFromVRAM) {
    if(io.readTransferFifoLength < 64) {
      io.readTransferFifo[io.readTransferFifoLength++] = value;
    }
    return;
  }

  if(io.mode == Mode::CopyToVRAM) {
    executeGP0(value, isThread);
    return;
  }

  if(!fifo.empty()) {
    fifo.write(value);
    return;
  }

  if(io.polylineActive && drawBusy()) {
    fifo.write(value);
    return;
  }
  if(!drawBusy() && fifo.empty() && latch.length == 0) {
    if(queue.gp0.empty()) {
      u8 cmd = value >> 24;
      if(cmd == 0x00) return;
      if(cmd >= 0xe3 && cmd <= 0xe5) {
        executeGP0(value, isThread);
        return;
      }
      if(!occupiesFifo(cmd)) return;
    }
    executeGP0(value, isThread);
    return;
  }

  if(drawBusy() || latch.length) {
    if(latch.ready) {
      fifo.write(value);
      return;
    }

    if(latch.length == 0) {
      u8 cmd = value >> 24;
      if(cmd == 0x00) return;
      if(cmd >= 0xe3 && cmd <= 0xe5) {
        executeGP0(value, isThread);
        return;
      }
      if(!occupiesFifo(cmd)) return;
      latch.target = commandLength(value);
    }

    latch.data[latch.length++] = value;
    if(latch.target && latch.length >= latch.target) latch.ready = 1;
    if(!latch.target && latch.length >= 3
    && (value & 0xf000'f000) == 0x5000'5000) latch.ready = 1;
    return;
  }

  if(ingress.length == 0) {
    u8 cmd = value >> 24;
    if(cmd >= 0xe3 && cmd <= 0xe5) {
      executeGP0(value, isThread);
      return;
    }
    if(!occupiesFifo(cmd)) return;
    ingress.target = commandLength(value);
    ingress.length = 1;
  } else {
    ingress.length++;
  }

  if(ingress.target && ingress.length >= ingress.target) {
    ingress.reset();
  } else if(!ingress.target && ingress.length >= 3
  && (value & 0xf000'f000) == 0x5000'5000) {
    ingress.reset();
  }

  fifo.write(value);
}

auto GPU::executeGP0(u32 value, bool isThread) -> void {
  if(io.mode == Mode::CopyToVRAM) {
    vram.mutex.lock();
    for(u32 loop : range(2)) {
      n10 x = io.copy.x + io.copy.px;
      n9  y = io.copy.y + io.copy.py;
      u16 pixel = vram2D[y & 511][x & 1023];
      if((pixel >> 15 & io.checkMaskBit) == 0) {
        vram2D[y & 511][x & 1023] = value | io.forceMaskBit << 15;
      }
      value >>= 16;
      if(++io.copy.px >= io.copy.width) {
        io.copy.px = 0;
        if(++io.copy.py >= io.copy.height) {
          io.copy.py = 0;
          io.mode = Mode::Normal;
          queueTextureCacheInvalidation();
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
  render.interlacedRendering = io.verticalResolution && io.interlace && !io.drawToDisplay;
  render.activeLineLSB = (io.displayStartY + (u32)io.displayField) & 1;
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
  render.texturePageBaseY = io.texturePageBaseY << 8 | io.texturePageBaseY2 << 9;
  render.texturePaletteX = io.texturePaletteX << 4;
  render.texturePaletteY = io.texturePaletteY << 0;
  render.texelMaskX = ~(io.textureWindowMaskX * 8);
  render.texelMaskY = ~(io.textureWindowMaskY * 8);
  render.texelOffsetX = (io.textureWindowOffsetX & io.textureWindowMaskX) * 8;
  render.texelOffsetY = (io.textureWindowOffsetY & io.textureWindowMaskY) * 8;
  render.textureFlipX = io.textureFlipX;
  render.textureFlipY = io.textureFlipY;

  auto setPalette = [&](u32 data) -> void {
    io.texturePaletteX = data >> 16 &  0x3f;
    io.texturePaletteY = data >> 22 & 0x1ff;

    render.texturePaletteX = io.texturePaletteX << 4;
    render.texturePaletteY = io.texturePaletteY << 0;
  };

  auto latchPage = [&](u32 data) -> void {
    auto previousPageBaseX = io.texturePageBaseX;
    auto previousPageBaseY = io.texturePageBaseY;
    auto previousPageBaseY2 = io.texturePageBaseY2;
    auto previousUses4bpp = io.textureDepth == 0;

    io.texturePageBaseX = data >> 16 & 15;
    io.texturePageBaseY = data >> 20 &  1;
    io.semiTransparency = data >> 21 &  3;
    io.textureDepth     = data >> 23 &  3;
    io.texturePageBaseY2 = (data >> 27 & 1) & io.texturePageBaseY2Enable;

    auto uses4bpp = io.textureDepth == 0;
    if(previousPageBaseX != io.texturePageBaseX
    || previousPageBaseY != io.texturePageBaseY
    || previousPageBaseY2 != io.texturePageBaseY2
    || previousUses4bpp != uses4bpp) {
      queueTextureCacheInvalidation();
    }

  };

  auto applyPageToRender = [&](u32 data) -> void {
    render.texturePageBaseX = (data >> 16 & 15) << 6;
    render.texturePageBaseY = (data >> 20 &  1) << 8 | io.texturePageBaseY2 << 9;
    render.semiTransparency =  data >> 21 &  3;
    render.textureDepth     =  data >> 23 &  3;
  };

  auto setPage = [&](u32 data) -> void {
    latchPage(data);
    applyPageToRender(data);
  };

  auto scheduleRender = [&]() -> void {
    if(fill.active) stepFill(fill.clocks);
    u32 cost = drawCycles(render);
    if(render.command == 0x02 && render.size.w && render.size.h) {
      renderer.synchronize();
      fill = {};
      fill.active = true;
      fill.restricted = render.interlacedRendering;
      fill.parity = render.activeLineLSB;
      fill.x = render.v0.x;
      fill.y = render.v0.y;
      fill.width = render.size.w;
      fill.height = render.size.h;
      fill.color = render.v0.to16();
      fill.clocks = max(1u, cost);
    } else {
      bool textured = (render.command & 4) && ((render.command >= 0x20 && render.command <= 0x3f)
                                                 || (render.command >= 0x60 && render.command <= 0x7f));
      if(textured) {
        //Rendering runs alongside the emulated base draw time. Its counters
        //are read only when the draw engine would otherwise become ready.
        textureCyclesAtQueue = renderer.textureCycles;
        textureBlendPixelsAtQueue = renderer.textureBlendPixels;
        pendingTextureTiming = true;
      }
      renderer.queue(render);
    }
    io.pcounter += cost;
    if(pendingTextureTiming && io.pcounter == 0) finishPendingRender();
  };

  auto finishRender = [&]() -> void {
    scheduleRender();
    queue.reset();
  };

  auto isPolylineTerminator = [](u32 data) -> bool {
    return (data & 0xf000f000) == 0x50005000;
  };

  //no operation
  if(command == 0x00) {
    return;
  }

  //clear cache
  if(command == 0x01) {
    queueCacheInvalidation();
    return;
  }

  //fill rectangle
  if(command == 0x02) {
    if(queue.write(value) < 3) return;
    u16 x = queue.data[1].bit( 0,15) & 0x3f0;
    u16 y = queue.data[1].bit(16,31) & 0x1ff;
    u16 width  = ((queue.data[2].bit( 0,15) & 0x3ff) + 15) & ~15;
    u16 height =  queue.data[2].bit(16,31) & 0x1ff;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(x, y);
    render.size = Size().setSize(width, height);
    return finishRender();
  }

  if(command == 0x03) {
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
    return finishRender();
  }

  //textured triangle
  if(command == 0x24 || command == 0x25 || command == 0x26 || command == 0x27) {
    auto length = queue.write(value);
    if(length < 7) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]).setTexel(queue.data[4]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[5]).setTexel(queue.data[6]);
    setPalette(queue.data[2]);
    setPage(queue.data[4]);
    return finishRender();
  }

  //monochrome quadrilateral
  if(command == 0x28 || command == 0x29 || command == 0x2a || command == 0x2b) {
    if(queue.write(value) < 5) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]);
    render.v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[4]);
    return finishRender();
  }

  //textured quadrilateral
  if(command == 0x2c || command == 0x2d || command == 0x2e || command == 0x2f) {
    auto length = queue.write(value);
    if(length == 7) latchPage(queue.data[4]);
    if(length < 9) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[3]).setTexel(queue.data[4]);
    render.v2 = Vertex().setColor(queue.data[0]).setPoint(queue.data[5]).setTexel(queue.data[6]);
    render.v3 = Vertex().setColor(queue.data[0]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    setPalette(queue.data[2]);
    applyPageToRender(queue.data[4]);
    return finishRender();
  }

  //shaded triangle
  if(command == 0x30 || command == 0x31 || command == 0x32 || command == 0x33) {
    if(queue.write(value) < 6) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    render.v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    return finishRender();
  }

  //shaded textured triangle
  if(command == 0x34 || command == 0x35 || command == 0x36 || command == 0x37) {
    auto length = queue.write(value);
    if(length < 9) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.v1 = Vertex().setColor(queue.data[3]).setPoint(queue.data[4]).setTexel(queue.data[5]);
    render.v2 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]).setTexel(queue.data[8]);
    setPalette(queue.data[2]);
    setPage(queue.data[5]);
    return finishRender();
  }

  //shaded quadrilateral
  if(command == 0x38 || command == 0x39 || command == 0x3a || command == 0x3b) {
    if(queue.write(value) < 8) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    render.v2 = Vertex().setColor(queue.data[4]).setPoint(queue.data[5]);
    render.v3 = Vertex().setColor(queue.data[6]).setPoint(queue.data[7]);
    return finishRender();
  }

  //shaded textured quadrilateral
  if(command == 0x3c || command == 0x3d || command == 0x3e || command == 0x3f) {
    auto length = queue.write(value);
    if(length == 9) latchPage(queue.data[5]);
    if(length < 12) return;
    render.v0 = Vertex().setColor(queue.data[ 0]).setPoint(queue.data[ 1]).setTexel(queue.data[ 2]);
    render.v1 = Vertex().setColor(queue.data[ 3]).setPoint(queue.data[ 4]).setTexel(queue.data[ 5]);
    render.v2 = Vertex().setColor(queue.data[ 6]).setPoint(queue.data[ 7]).setTexel(queue.data[ 8]);
    render.v3 = Vertex().setColor(queue.data[ 9]).setPoint(queue.data[10]).setTexel(queue.data[11]);
    setPalette(queue.data[2]);
    applyPageToRender(queue.data[5]);
    return finishRender();
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
    return finishRender();
  }

  //monochrome poly-line
  if(command == 0x48 || command == 0x49 || command == 0x4a || command == 0x4b
  || command == 0x4c || command == 0x4d || command == 0x4e || command == 0x4f) {
    if(queue.counterX && queue.length == 2 && isPolylineTerminator(value)) {
      io.polylineActive = 0;
      return queue.reset();
    }
    io.polylineActive = 1;
    auto length = queue.write(value);
    if(length < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[0]).setPoint(queue.data[2]);
    scheduleRender();
    queue.data[1] = queue.data[2];
    queue.length = 2;
    queue.counterX++;
    return;
  }

  //shaded line
  if(command == 0x50 || command == 0x51 || command == 0x52 || command == 0x53
  || command == 0x54 || command == 0x55 || command == 0x56 || command == 0x57) {
    if(queue.write(value) < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    return finishRender();
  }

  //shaded poly-line
  if(command == 0x58 || command == 0x59 || command == 0x5a || command == 0x5b
  || command == 0x5c || command == 0x5d || command == 0x5e || command == 0x5f) {
    if(queue.counterX && queue.length == 2 && isPolylineTerminator(value)) {
      io.polylineActive = 0;
      return queue.reset();
    }
    io.polylineActive = 1;
    auto length = queue.write(value);
    if(length < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.v1 = Vertex().setColor(queue.data[2]).setPoint(queue.data[3]);
    scheduleRender();
    queue.data[0] = queue.data[2];
    queue.data[1] = queue.data[3];
    queue.length = 2;
    queue.counterX++;
    return;
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
    return finishRender();
  }

  //textured rectangle (variable size)
  if(command == 0x64 || command == 0x65 || command == 0x66 || command == 0x67) {
    if(queue.write(value) < 4) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(queue.data[3]);
    setPalette(queue.data[2]);
    return finishRender();
  }

  //monochrome rectangle (1x1 size)
  if(command == 0x68 || command == 0x69 || command == 0x6a || command == 0x6b) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(1, 1);
    return finishRender();
  }

  //textured rectangle (1x1 size)
  if(command == 0x6c || command == 0x6d || command == 0x6e || command == 0x6f) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(1, 1);
    setPalette(queue.data[2]);
    return finishRender();
  }

  //monochrome rectangle (8x8 size)
  if(command == 0x70 || command == 0x71 || command == 0x72 || command == 0x73) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(8, 8);
    return finishRender();
  }

  //textured rectangle (8x8 size)
  if(command == 0x74 || command == 0x75 || command == 0x76 || command == 0x77) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(8, 8);
    setPalette(queue.data[2]);
    return finishRender();
  }

  //monochrome rectangle (16x16 size)
  if(command == 0x78 || command == 0x79 || command == 0x7a || command == 0x7b) {
    if(queue.write(value) < 2) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]);
    render.size = Size().setSize(16, 16);
    return finishRender();
  }

  //textured rectangle (16x16 size)
  if(command == 0x7c || command == 0x7d || command == 0x7e || command == 0x7f) {
    if(queue.write(value) < 3) return;
    render.v0 = Vertex().setColor(queue.data[0]).setPoint(queue.data[1]).setTexel(queue.data[2]);
    render.size = Size().setSize(16, 16);
    setPalette(queue.data[2]);
    return finishRender();
  }

  //copy rectangle (VRAM to VRAM)
  if(command >= 0x80 && command <= 0x9f) {
    if(queue.write(value) < 4) return;
    renderer.synchronize();
    invalidateTextureCache();
    u16 sourceX = queue.data[1].bit( 0,15);
    u16 sourceY = queue.data[1].bit(16,31);
    u16 targetX = queue.data[2].bit( 0,15);
    u16 targetY = queue.data[2].bit(16,31);
    u16 width   = queue.data[3].bit( 0,15) & 0x3ff;
    u16 height  = queue.data[3].bit(16,31) & 0x1ff;
    if(!width ) width  = 1024;
    if(!height) height =  512;
    io.pcounter += (u32)(width * height * 2.172);
    for(u32 y : range(height)) {
      for(u32 x : range(0, width, 0x80)) {
        u16 buffer[128];
        u32 chunkWidth = min<u32>(0x80, width - x);

        for(u32 cx : range(chunkWidth)) {
          buffer[cx] = vram2D[n9(y + sourceY) & 511][n10(x + cx + sourceX) & 1023];
        }

        for(u32 cx : range(chunkWidth)) {
          u16 pixel = vram2D[n9(y + targetY) & 511][n10(x + cx + targetX) & 1023];
          if((pixel >> 15 & io.checkMaskBit) == 0) {
            vram2D[n9(y + targetY) & 511][n10(x + cx + targetX) & 1023] = buffer[cx] | io.forceMaskBit << 15;
          }
        }
      }
    }
    return queue.reset();
  }

  //copy rectangle (CPU to VRAM)
  if(command >= 0xa0 && command <= 0xbf) {
    if(queue.write(value) < 3) return;
    renderer.synchronize();
    invalidateTextureCache();
    io.copy.x      = (queue.data[1].bit( 0,15) & 1023);
    io.copy.y      = (queue.data[1].bit(16,31) &  511);
    io.copy.width  = queue.data[2].bit( 0,15) & 1023;
    io.copy.height = queue.data[2].bit(16,31) &  511;
    if(!io.copy.width ) io.copy.width  = 1024;
    if(!io.copy.height) io.copy.height =  512;
    io.copy.px     = 0;
    io.copy.py     = 0;
    io.mode        = Mode::CopyToVRAM;
    return queue.reset();
  }

  //copy rectangle (VRAM to CPU)
  if(command >= 0xc0 && command <= 0xdf) {
    if(queue.write(value) < 3) return;
    renderer.synchronize();
    invalidateTextureCache();
    io.copy.x      = (queue.data[1].bit( 0,15) & 1023);
    io.copy.y      = (queue.data[1].bit(16,31) &  511);
    io.copy.width  = queue.data[2].bit( 0,15) & 1023;
    io.copy.height = queue.data[2].bit(16,31) &  511;
    if(!io.copy.width ) io.copy.width  = 1024;
    if(!io.copy.height) io.copy.height =  512;
    io.copy.px     = 0;
    io.copy.py     = 0;
    io.mode        = io.copy.width && io.copy.height ? Mode::CopyFromVRAM : Mode::Normal;
    io.vramReadLatchValid = 0;
    io.vramReadWordValid = 0;
    io.vramReadWord = cpu.cycleCount();
    io.readTransferFifoLength = 0;
    return queue.reset();
  }

  //no operation
  if(command == 0xe0) {
    return;
  }

  //draw mode
  if(command == 0xe1) {
    auto previousPageBaseX = io.texturePageBaseX;
    auto previousPageBaseY = io.texturePageBaseY;
    auto previousPageBaseY2 = io.texturePageBaseY2;
    auto previousUses4bpp = io.textureDepth == 0;

    io.texturePageBaseX = data.bit( 0, 3);
    io.texturePageBaseY = data.bit( 4);
    io.semiTransparency = data.bit( 5, 6);
    io.textureDepth     = data.bit( 7, 8);
    io.dithering        = data.bit( 9);
    io.drawToDisplay    = data.bit(10);
    io.texturePageBaseY2 = data.bit(11) & io.texturePageBaseY2Enable;
    io.textureFlipX     = data.bit(12);
    io.textureFlipY     = data.bit(13);

    auto uses4bpp = io.textureDepth == 0;
    if(previousPageBaseX != io.texturePageBaseX
    || previousPageBaseY != io.texturePageBaseY
    || previousPageBaseY2 != io.texturePageBaseY2
    || previousUses4bpp != uses4bpp) {
      queueTextureCacheInvalidation();
    }

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

  if(command >= 0xf0 && command <= 0xff) {
    return;
  }
}

auto GPU::stepFill(u32 clocks) -> void {
  if(!fill.active) return;
  fill.elapsed = min(fill.clocks, fill.elapsed + min(clocks, fill.clocks));
  u32 target = (u64)fill.width * fill.height * fill.elapsed / fill.clocks;
  vram.mutex.lock();

  u32 written = fill.written;
  if(written < target) {
    u32 x = written % fill.width;
    u32 y = written / fill.width;
    do {
      if(!fill.restricted || ((fill.y + y) & 1) != fill.parity) vram2D[(fill.y + y) & 511][(fill.x + x) & 1023] = fill.color;
      written++;
      if(++x == fill.width) { x = 0; y++; }
    } while(written < target);
    fill.written = written;
  }

  vram.mutex.unlock();
  if(fill.elapsed == fill.clocks) fill.active = false;
}
