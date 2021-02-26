auto GPU::generateTables() -> void {
  static constexpr s8 table[4][4] = {
    {-4, +0, -3, +1},
    {+2, -2, +3, -1},
    {-3, +1, -4, +0},
    {+3, -1, +2, -2},
  };
  for(u32 y : range(4)) {
    for(u32 x : range(4)) {
      for(s32 c : range(256)) {
        ditherTable[y][x][c] = uclamp<8>(c + table[y][x]);
      }
    }
  }

  for(u32 a : range(2)) {
    for(u32 r : range(32)) {
      for(u32 g : range(32)) {
        for(u32 b : range(32)) {
          Color color;
          color.r = r << 3 | r >> 2;
          color.g = g << 3 | g >> 2;
          color.b = b << 3 | b >> 2;
        //color.a = a ? 0xff : 0x00;
          Color::table[a << 15 | b << 10 | g << 5 | r << 0] = color;
        }
      }
    }
  }
}

//compute barycentric weight (area) between three vertices
auto GPU::Render::weight(Point a, Point b, Point c) const -> s32 {
  return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

auto GPU::Render::origin(Point a, Point b, Point c, s32 d[3], f32 area, s32 bias[3]) const -> f32 {
  f32 x = (b.x * c.y - c.x * b.y) * d[0] - bias[0];
  f32 y = (c.x * a.y - a.x * c.y) * d[1] - bias[1];
  f32 z = (a.x * b.y - b.x * a.y) * d[2] - bias[2];
  return (x + y + z) / area;
}

auto GPU::Render::delta(Point a, Point b, Point c, s32 d[3], f32 area) const -> Delta {
  f32 x = (b.y - c.y) * d[0] + (c.y - a.y) * d[1] + (a.y - b.y) * d[2];
  f32 y = (c.x - b.x) * d[0] + (a.x - c.x) * d[1] + (b.x - a.x) * d[2];
  return {x / area, y / area};
}

auto GPU::Render::texel(Point p) const -> u16 {
  u16 px = texturePaletteX;
  u16 py = texturePaletteY;
  s32 bx = texturePageBaseX;
  s32 by = texturePageBaseY;
  s32 tx = u8(p.x) & texelMaskX | texelOffsetX;
  s32 ty = u8(p.y) & texelMaskY | texelOffsetY;

  if(textureDepth == 0) {  //4bpp
    u16 index = gpu.vram2D[ty + by & 511][tx / 4 + bx & 1023];
    u16 entry = index >> (tx & 3) * 4 & 15;
    return gpu.vram2D[py][px + entry & 1023];
  }

  if(textureDepth == 1) {  //8bpp
    u16 index = gpu.vram2D[ty + by & 511][tx / 2 + bx & 1023];
    u16 entry = index >> (tx & 1) * 8 & 255;
    return gpu.vram2D[py][px + entry & 1023];
  }

  if(textureDepth == 2) {  //16bpp
    return gpu.vram2D[ty + by & 511][tx + bx & 1023];
  }

  return 0;
}

auto GPU::Render::dither(Point p, Color c) const -> Color {
  if(dithering) {
    auto dither = gpu.ditherTable[p.y & 3][p.x & 3];
    c.r = dither[c.r];
    c.g = dither[c.g];
    c.b = dither[c.b];
  }
  return c;
}

auto GPU::Render::modulate(Color above, Color below) const -> Color {
  above.r = min(255, above.r * below.r >> 7);
  above.g = min(255, above.g * below.g >> 7);
  above.b = min(255, above.b * below.b >> 7);
  return above;
}

auto GPU::Render::alpha(Color above, Color below) const -> Color {
  switch(semiTransparency) {
  case 0:
    above.r = below.r + above.r >> 1;
    above.g = below.g + above.g >> 1;
    above.b = below.b + above.b >> 1;
    break;
  case 1:
    above.r = min(255, below.r + above.r);
    above.g = min(255, below.g + above.g);
    above.b = min(255, below.b + above.b);
    break;
  case 2:
    above.r = max(0, (s32)below.r - above.r);
    above.g = max(0, (s32)below.g - above.g);
    above.b = max(0, (s32)below.b - above.b);
    break;
  case 3:
    above.r = min(255, below.r + (above.r >> 2));
    above.g = min(255, below.g + (above.g >> 2));
    above.b = min(255, below.b + (above.b >> 2));
    break;
  }
  return above;
}

template<u32 Flags>
auto GPU::Render::pixel(Point point, Color rgb, Point uv) -> void {
  Color above;
  bool transparent;
  bool maskBit = forceMaskBit;

  if constexpr(Flags & Texture) {
    u16 pixel = texel(uv);
    if(!pixel) return;

    transparent = pixel >> 15;
    maskBit |= transparent;
    if constexpr(Flags & Raw) {
      above = Color::from16(pixel);
    } else if constexpr(true) {
      above = modulate(Color::from16(pixel), rgb);
    }
  } else if constexpr(true) {
    transparent = true;
    above = rgb;
  }
  if constexpr(Flags & Dither) {
    above = dither(point, above);
  }

  u16 input = gpu.vram2D[point.y][point.x];
  Color below = Color::from16(input);

  if constexpr(Flags & Alpha) {
    if(transparent) above = alpha(above, below);
  }

  if(input >> 15 & checkMaskBit) return;
  gpu.vram2D[point.y][point.x] = above.to16() | maskBit << 15;
}

template<u32 Flags>
auto GPU::Render::line() -> void {
  v0.x += drawingAreaOffsetX, v0.y += drawingAreaOffsetY;
  v1.x += drawingAreaOffsetX, v1.y += drawingAreaOffsetY;

  v0.x = std::clamp(v0.x, drawingAreaOriginX1, drawingAreaOriginX2);
  v0.y = std::clamp(v0.y, drawingAreaOriginY1, drawingAreaOriginY2);
  v1.x = std::clamp(v1.x, drawingAreaOriginX1, drawingAreaOriginX2);
  v1.y = std::clamp(v1.y, drawingAreaOriginY1, drawingAreaOriginY2);

  Point d = {v1.x - v0.x, v1.y - v0.y};
  s32 steps = abs(d.x) > abs(d.y) ? abs(d.x) : abs(d.y);
  if(steps == 0) {
    if(v0.x == v1.x && v0.y == v1.y) {
      return pixel<Flags>(v0, v0);
    } else {
      debug(unimplemented, "GPU::renderLine(steps=0)");
      return;
    }
  }

  Point s = {(d.x << 16) / steps, (d.y << 16) / steps};
  Point p = {v0.x << 16, v0.y << 16};

  u32 pixels = 0;
  for(u16 step : range(steps)) {
    pixel<Flags | Dither>({p.x >> 16, p.y >> 16}, v0);
    p.x += s.x, p.y += s.y;
    pixels++;
  }

//io.pcounter += cost<Flags | Line>(pixels);

  if constexpr(Flags & Shade) {
    debug(unimplemented, "ShadedLine");
  }
}

template<u32 Flags>
auto GPU::Render::triangle() -> void {
  static constexpr u32 Dithering = ((
    (Flags & Shade) || ((Flags & Texture) && (Flags & Alpha))
  ) && !(Flags & Rectangle)) ? Dither : 0;

  v0.x += drawingAreaOffsetX, v0.y += drawingAreaOffsetY;
  v1.x += drawingAreaOffsetX, v1.y += drawingAreaOffsetY;
  v2.x += drawingAreaOffsetX, v2.y += drawingAreaOffsetY;

  Point vmin{min(v0.x, v1.x, v2.x), min(v0.y, v1.y, v2.y)};
  Point vmax{max(v0.x, v1.x, v2.x), max(v0.y, v1.y, v2.y)};

  //reject triangles larger than the VRAM size
  if(vmax.x - vmin.x > 1024 || vmax.y - vmin.y > 512) return;

  //clip rendering to drawing area
  vmin.x = std::clamp(vmin.x, drawingAreaOriginX1, drawingAreaOriginX2);
  vmin.y = std::clamp(vmin.y, drawingAreaOriginY1, drawingAreaOriginY2);
  vmax.x = std::clamp(vmax.x, drawingAreaOriginX1, drawingAreaOriginX2);
  vmax.y = std::clamp(vmax.y, drawingAreaOriginY1, drawingAreaOriginY2);

  s32 area = weight(v0, v1, v2);  //<0 = counter-clockwise; 0 = colinear, >0 = clockwise
  if(area == 0) return;  //do not render colinear triangles
  if(area < 0) swap(v1, v2), area = -area;  //make clockwise

  Point d0{v1.y - v2.y, v2.x - v1.x};
  Point d1{v2.y - v0.y, v0.x - v2.x};
  Point d2{v0.y - v1.y, v1.x - v0.x};

  s32 bias[3];  //avoid drawing overlapping top-left edges of triangles
  bias[0] = -(d0.x < 0 || d0.x == 0 && d0.y < 0);
  bias[1] = -(d1.x < 0 || d1.x == 0 && d1.y < 0);
  bias[2] = -(d2.x < 0 || d2.x == 0 && d2.y < 0);

  Point p0, p1, p2;
  Delta dr, dg, db, du, dv;
  Delta pr, pg, pb, pu, pv;

  p0.y = weight(v1, v2, vmin);
  p1.y = weight(v2, v0, vmin);
  p2.y = weight(v0, v1, vmin);

  if constexpr(Flags & Shade) {
    s32 r[3] = {v0.r, v1.r, v2.r};
    s32 g[3] = {v0.g, v1.g, v2.g};
    s32 b[3] = {v0.b, v1.b, v2.b};
    dr = delta(v0, v1, v2, r, area);
    dg = delta(v0, v1, v2, g, area);
    db = delta(v0, v1, v2, b, area);
    pr.y = origin(v0, v1, v2, r, area, bias) + dr.x * vmin.x + dr.y * vmin.y;
    pg.y = origin(v0, v1, v2, g, area, bias) + dg.x * vmin.x + dg.y * vmin.y;
    pb.y = origin(v0, v1, v2, b, area, bias) + db.x * vmin.x + db.y * vmin.y;
  } else if constexpr(true) {
    pr.x = v0.r;
    pg.x = v0.g;
    pb.x = v0.b;
  }

  if constexpr(Flags & Texture) {
    s32 u[3] = {v0.u, v1.u, v2.u};
    s32 v[3] = {v0.v, v1.v, v2.v};
    du = delta(v0, v1, v2, u, area);
    dv = delta(v0, v1, v2, v, area);
    pu.y = origin(v0, v1, v2, u, area, bias) + du.x * vmin.x + du.y * vmin.y;
    pv.y = origin(v0, v1, v2, v, area, bias) + dv.x * vmin.x + dv.y * vmin.y;
  } else if constexpr(true) {
    pu.x = 0;
    pv.x = 0;
  }

  u32 pixels = 0;
  Point vp{vmin};
  for(vp.y = vmin.y; vp.y <= vmax.y; vp.y++) {
    p0.x = p0.y, p1.x = p1.y, p2.x = p2.y;
    if constexpr(Flags & Shade) pr.x = pr.y, pg.x = pg.y, pb.x = pb.y;
    if constexpr(Flags & Texture) pu.x = pu.y, pv.x = pv.y;

    for(vp.x = vmin.x; vp.x <= vmax.x; vp.x++) {
      if((p0.x + bias[0] | p1.x + bias[1] | p2.x + bias[2]) >= 0) {
        pixel<Flags | Dithering>(vp, {pr.x, pg.x, pb.x}, {pu.x, pv.x});
        pixels++;
      }

      p0.x += d0.x, p1.x += d1.x, p2.x += d2.x;
      if constexpr(Flags & Shade) pr.x += dr.x, pg.x += dg.x, pb.x += db.x;
      if constexpr(Flags & Texture) pu.x += du.x, pv.x += dv.x;
    }

    p0.y += d0.y, p1.y += d1.y, p2.y += d2.y;
    if constexpr(Flags & Shade) pr.y += dr.y, pg.y += dg.y, pb.y += db.y;
    if constexpr(Flags & Texture) pu.y += du.y, pv.y += dv.y;
  }

//io.pcounter += cost<Flags>(pixels);
}

template<u32 Flags>
auto GPU::Render::quadrilateral() -> void {
  auto c1 = v1, c2 = v2, c3 = v3;
  triangle<Flags>();
  v0 = c1, v1 = c2, v2 = c3;
  triangle<Flags>();
}

template<u32 Flags>
auto GPU::Render::rectangle() -> void {
  v1 = Vertex(v0).setPoint(v0.x + size.w, v0.y).setTexel(v0.u + size.w, v0.v);
  v2 = Vertex(v0).setPoint(v0.x, v0.y + size.h).setTexel(v0.u, v0.v + size.h);
  v3 = Vertex(v0).setPoint(v0.x + size.w, v0.y + size.h).setTexel(v0.u + size.w, v0.v + size.h);
  quadrilateral<Flags | Rectangle>();
}

template<u32 Flags>
auto GPU::Render::fill() -> void {
  auto color = v0.to16();
  for(u32 y : range(size.h)) {
    for(u32 x : range(size.w)) {
      gpu.vram2D[y + v0.y & 511][x + v0.x & 1023] = color;
    }
  }
//io.pcounter += cost<Flags>(size.width * size.height);
}

template<u32 Flags>
auto GPU::Render::cost(u32 pixels) const -> u32 {
  //for now, do not emulate GPU overhead timing ...
  return 1;

  //below numbers are based off timings from ps1-tests/gpu/bandwidth.
  //the numbers are off by a factor of three in other games, however.
  if(Flags & Fill) {
    return pixels * 0.081;
  } else if(Flags & Line) {
    //todo: need to benchmark line drawing speeds
    return pixels * 0.500;
  } else if(Flags & Rectangle) {
    if(Flags & Texture) {
      if(Flags & Alpha) {
        return pixels * 1.497;
      } else {
        return pixels * 1.188;
      }
    } else {
      if(Flags & Alpha) {
        return pixels * 0.797;
      } else {
        return pixels * 0.530;
      }
    }
  } else {
    if(Flags & Texture) {
      if(Flags & Alpha) {
        return pixels * 3.463;
      } else {
        return pixels * 2.980;
      }
    } else {
      if(Flags & Alpha) {
        return pixels * 0.812;
      } else {
        return pixels * 0.538;
      }
    }
  }
}

auto GPU::Render::execute() -> void {
  switch(command) {
  case 0x02: return fill<Fill>();
  case 0x20: return triangle<None>();
  case 0x21: return triangle<Raw>();
  case 0x22: return triangle<Alpha>();
  case 0x23: return triangle<Alpha | Raw>();
  case 0x24: return triangle<Texture>();
  case 0x25: return triangle<Texture | Raw>();
  case 0x26: return triangle<Texture | Alpha>();
  case 0x27: return triangle<Texture | Alpha | Raw>();
  case 0x28: return quadrilateral<None>();
  case 0x29: return quadrilateral<Raw>();
  case 0x2a: return quadrilateral<Alpha>();
  case 0x2b: return quadrilateral<Alpha | Raw>();
  case 0x2c: return quadrilateral<Texture>();
  case 0x2d: return quadrilateral<Texture | Raw>();
  case 0x2e: return quadrilateral<Texture | Alpha>();
  case 0x2f: return quadrilateral<Texture | Alpha | Raw>();
  case 0x30: return triangle<Shade>();
  case 0x31: return triangle<Shade | Raw>();
  case 0x32: return triangle<Shade | Alpha>();
  case 0x33: return triangle<Shade | Alpha | Raw>();
  case 0x34: return triangle<Shade | Texture>();
  case 0x35: return triangle<Shade | Texture | Raw>();
  case 0x36: return triangle<Shade | Texture | Alpha>();
  case 0x37: return triangle<Shade | Texture | Alpha | Raw>();
  case 0x38: return quadrilateral<Shade>();
  case 0x39: return quadrilateral<Shade | Raw>();
  case 0x3a: return quadrilateral<Shade | Alpha>();
  case 0x3b: return quadrilateral<Shade | Alpha | Raw>();
  case 0x3c: return quadrilateral<Shade | Texture>();
  case 0x3d: return quadrilateral<Shade | Texture | Raw>();
  case 0x3e: return quadrilateral<Shade | Texture | Alpha>();
  case 0x3f: return quadrilateral<Shade | Texture | Alpha | Raw>();
  case 0x40: case 0x41: case 0x44: case 0x45: return line<None>();
  case 0x42: case 0x43: case 0x46: case 0x47: return line<Alpha>();
  case 0x48: case 0x49: case 0x4c: case 0x4d: return line<None>();
  case 0x4a: case 0x4b: case 0x4e: case 0x4f: return line<Alpha>();
  case 0x50: case 0x51: case 0x54: case 0x55: return line<Shade>();
  case 0x52: case 0x53: case 0x56: case 0x57: return line<Shade | Alpha>();
  case 0x58: case 0x59: case 0x5c: case 0x5d: return line<Shade>();
  case 0x5a: case 0x5b: case 0x5e: case 0x5f: return line<Shade | Alpha>();
  case 0x60: case 0x68: case 0x70: case 0x78: return rectangle<None>();
  case 0x61: case 0x69: case 0x71: case 0x79: return rectangle<Raw>();
  case 0x62: case 0x6a: case 0x72: case 0x7a: return rectangle<Alpha>();
  case 0x63: case 0x6b: case 0x73: case 0x7b: return rectangle<Alpha | Raw>();
  case 0x64: case 0x6c: case 0x74: case 0x7c: return rectangle<Texture>();
  case 0x65: case 0x6d: case 0x75: case 0x7d: return rectangle<Texture | Raw>();
  case 0x66: case 0x6e: case 0x76: case 0x7e: return rectangle<Texture | Alpha>();
  case 0x67: case 0x6f: case 0x77: case 0x7f: return rectangle<Texture | Alpha | Raw>();
  }
}

auto GPU::Renderer::queue(Render& render) -> void {
  if constexpr(Accuracy::GPU::Threaded) {
    fifo.await_write(render);
  } else if constexpr(true) {
    render.execute();
  }
}

auto GPU::Renderer::main(uintptr_t) -> void {
  while(true) {
    auto render = fifo.await_read();
    self.vram.mutex.lock();
    render.execute();
    self.vram.mutex.unlock();
    if(render.command == 0x100) thread::exit();
  }
}

auto GPU::Renderer::kill() -> void {
  if constexpr(Accuracy::GPU::Threaded) {
    Render kill;
    kill.command = 0x100;
    queue(kill);
    handle.join();
  }
}

auto GPU::Renderer::power() -> void {
  if constexpr(Accuracy::GPU::Threaded) {
    kill();
    fifo.flush();
    handle = thread::create({&GPU::Renderer::main, &self.renderer});
  }
}
