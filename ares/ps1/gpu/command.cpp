// Command execution uses guest GPU clocks. Host rasterization may finish earlier
// or later without changing the input FIFO or DMA request.
auto GPU::commandWords(u32 command) const -> u32 {
  if(command == 0x02) return 3;
  if(command >= 0x20 && command < 0x40) {
    u32 vertices = command & 8 ? 4 : 3;
    return vertices * (1 + bool(command & 4) + bool(command & 16)) + !(command & 16);
  }
  if(command >= 0x40 && command < 0x60) return command & 16 ? 4 : 3;
  if(command >= 0x60 && command < 0x80) return 2 + bool(command & 4) + !(command & 0x18);
  if(command >= 0x80 && command < 0xa0) return 4;
  if(command >= 0xa0 && command < 0xe0) return 3;
  return 1;
}

auto GPU::receiveDMA() const -> bool {
  if(io.mode == Mode::CopyFromVRAM || input.size >= 16) return false;
  if(io.mode == Mode::CopyToVRAM || !queue.gp0.empty()) return true;
  if(!input.size) return true;
  return input.size < commandWords(input.data[input.read] >> 24);
}

auto GPU::writeGP0(u32 value) -> void {
  if(input.size == Input::Capacity) {
    debug(unverified, "GPU input FIFO overflow");
    return;
  }
  input.data[(input.read + input.size) & (Input::Capacity - 1)] = value;
  input.size++;
  drainCommands();
}

auto GPU::drainCommands() -> void {
  while(input.size && io.pcounter == 0 && io.mode != Mode::CopyFromVRAM) {
    u32 value = input.data[input.read];
    input.read = (input.read + 1) & (Input::Capacity - 1);
    input.size--;
    executeGP0(value);
  }
}

auto GPU::advanceCommands(u32 clocks) -> void {
  drainCommands();
  while(io.pcounter > 0) {
    u32 elapsed = std::min(clocks, (u32)io.pcounter);
    io.pcounter -= elapsed;
    clocks -= elapsed;
    if(io.pcounter) break;
    drainCommands();
    if(!clocks) break;
  }
}

// Reference-derived estimates: DuckStation b0f7c5c GPU draw-command costs.
// Triangle clipping is an area estimate, not a hardware pixel-clock measurement.
auto GPU::Render::clocks() const -> u32 {
  if(command == 0x02) {
    u32 width = size.w;
    return 46 + (width / 8 + 9) * size.h;
  }
  bool textured = command & 4;
  bool shaded = command & 16;
  bool blended = (command & 2) || checkMaskBit;
  auto point = [&](const Vertex& v) -> Point {
    return {(i11)(v.x + drawingAreaOffsetX), (i11)(v.y + drawingAreaOffsetY)};
  };
  auto area = [&](Point a, Point b, Point c) -> u32 {
    if(drawingAreaOriginX1 > drawingAreaOriginX2 || drawingAreaOriginY1 > drawingAreaOriginY2) return 0;
    for(auto p : {&a, &b, &c}) {
      p->x = std::clamp(p->x, drawingAreaOriginX1, drawingAreaOriginX2);
      p->y = std::clamp(p->y, drawingAreaOriginY1, drawingAreaOriginY2);
    }
    u32 pixels = abs(weight(a, b, c)) / 2;
    if(textured) pixels *= 2;
    if(blended) pixels += (pixels + 1) / 2;
    if(interlaced) pixels /= 2;
    return pixels;
  };
  if(command >= 0x20 && command < 0x40) {
    u32 setup = shaded ? (textured ? 496 : 334) : (textured ? 226 : 46);
    u32 ticks = setup + area(point(v0), point(v1), point(v2));
    if(command & 8) ticks += 36 + area(point(v2), point(v1), point(v3));
    return ticks;
  }
  if(command >= 0x40 && command < 0x80) {
    auto a = point(v0);
    auto b = point(v1);
    s32 left = a.x, top = a.y, right, bottom;
    if(command < 0x60) {
      left = std::min(a.x, b.x);
      top = std::min(a.y, b.y);
      right = std::max(a.x, b.x) + 1;
      bottom = std::max(a.y, b.y) + 1;
    } else {
      right = left + (size.w & 1023);
      bottom = top + (size.h & 511);
    }
    u32 width = std::max(0, std::min(right, drawingAreaOriginX2 + 1) - std::max(left, drawingAreaOriginX1));
    u32 height = std::max(0, std::min(bottom, drawingAreaOriginY2 + 1) - std::max(top, drawingAreaOriginY1));
    u32 setup = continuation ? 0 : 16;
    if(!width || !height) return setup;
    if(command < 0x60) {
      if(interlaced) height = std::max(1u, height / 2);
      return setup + std::max(width, height);
    }
    u32 row = width;
    if(textured) {
      if(textureDepth == 0) row += width;
      else if(width > 128) row += (width / (textureDepth == 1 ? 4 : 2)) * 8;
      else if(width * height > (textureDepth == 1 ? 2048 : 1024)) {
        row += (width / 4) * (textureDepth == 1 ? 4 : 8) * (128 / width);
      } else row += width;
    }
    if(blended) row += (width + 1) / 2;
    if(interlaced) height = std::max(1u, height / 2);
    return 16 + row * height;
  }
  return 0;
}
