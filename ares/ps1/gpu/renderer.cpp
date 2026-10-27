
constexpr auto clip = [](auto x, auto lo, auto hi) {
  return std::clamp(x, lo <= hi ? lo : hi, hi >= lo ? hi : lo);
};

static constexpr double CpuToGpuClocks = 53'693'175.0 / 33'868'800.0;


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

auto GPU::Render::drawableArea(Point a, Point b, Point c) const -> s32 {
  Point vmin{min(a.x, b.x, c.x), min(a.y, b.y, c.y)};
  Point vmax{max(a.x, b.x, c.x), max(a.y, b.y, c.y)};
  if(vmax.x - vmin.x >= 1024 || vmax.y - vmin.y >= 512) return 0;
  return weight(a, b, c);
}

auto GPU::Render::delta(Point a, Point b, Point c, s32 d[3], s32 area) const -> Delta {
  s64 x = s64(b.y - c.y) * d[0] + s64(c.y - a.y) * d[1] + s64(a.y - b.y) * d[2];
  s64 y = s64(c.x - b.x) * d[0] + s64(a.x - c.x) * d[1] + s64(b.x - a.x) * d[2];
  return {s32((x << 12) / area), s32((y << 12) / area)};
}

auto GPU::invalidateTextureCache() -> void {
  for(auto& entry : textureCache) entry.tag = ~0u;
}

auto GPU::invalidateCLUTCache() -> void {
  clutCacheTag4 = ~0u;
  clutCacheTag8 = ~0u;
}

auto GPU::invalidateCache() -> void {
  invalidateTextureCache();
  invalidateCLUTCache();
}

auto GPU::queueTextureCacheInvalidation() -> void {
  Render render;
  render.command = 0x102;
  renderer.queue(render);
}

auto GPU::queueCacheInvalidation() -> void {
  Render render;
  render.command = 0x103;
  renderer.queue(render);
}

auto GPU::Render::loadCLUTCache() const -> void {
  if(textureDepth >= 2) return;

  u32 tag = (texturePaletteY << 6 | texturePaletteX >> 4) & 0x7fff;
  auto load = [&](u32 count) -> void {
    gpu.renderer.textureCycles += (u32)(count * CpuToGpuClocks);
    for(u32 index : range(count)) {
      gpu.clutCache[index] = gpu.vram2D[texturePaletteY & 511][texturePaletteX + index & 1023];
    }
  };

  if(textureDepth == 0) {
    if(gpu.clutCacheTag4 != tag) {
      gpu.clutCacheTag4 = tag;
      load(16);
    }
    return;
  }

  if(gpu.clutCacheTag8 != tag) {
    gpu.clutCacheTag8 = tag;
    gpu.clutCacheTag4 = tag;
    load(256);
  } else if(gpu.clutCacheTag4 != tag) {
    gpu.clutCacheTag4 = tag;
    load(16);
  }
}

auto GPU::Render::texel(Point p) const -> u16 {
  s32 bx = texturePageBaseX;
  s32 by = texturePageBaseY;
  s32 tx = u8(p.x);
  s32 ty = u8(p.y) & texelMaskY | texelOffsetY;

  if(texelErrorMask && (tx & texelErrorMask) == 0) {
    tx = u8(tx - 1);
    tx = u8(tx & texelMaskX | texelOffsetX);
    if(textureDepth == 0 && !(tx & 8)) {
      tx = u8(tx + 1);
    }
  } else {
    tx = u8(tx & texelMaskX | texelOffsetX);
  }

  auto readTexture = [&](u32 byteX, u32 y) -> u16 {
    byteX &= 2047;
    u32 cacheIndex = textureDepth == 0
      ? ((byteX >> 3) & 0x03) | ((y << 2) & 0xfc)
      : ((byteX >> 3) & 0x07) | ((y << 3) & 0xf8);
    auto& entry = gpu.textureCache[cacheIndex];
    u32 tag = (y << 11) | (byteX & 0x7f8);

    if(entry.tag != tag) {
      gpu.renderer.textureCycles += 13;
      u32 base = byteX >> 1 & ~3u;
      if(tag >= 0x100000) {
        entry.data[0] = 0x7fff;
        entry.data[1] = 0x7fff;
        entry.data[2] = 0x7fff;
        entry.data[3] = 0x7fff;
      } else {
        entry.data[0] = gpu.vram2D[y & 511][base + 0 & 1023];
        entry.data[1] = gpu.vram2D[y & 511][base + 1 & 1023];
        entry.data[2] = gpu.vram2D[y & 511][base + 2 & 1023];
        entry.data[3] = gpu.vram2D[y & 511][base + 3 & 1023];
      }
      entry.tag = tag;
    }

    return entry.data[byteX >> 1 & 3];
  };

  auto readCLUT = [&](u32 entry) -> u16 {
    loadCLUTCache();
    return gpu.clutCache[entry];
  };

  if(textureDepth == 0) {  //4bpp
    u32 byteX = (tx >> 1) + (bx << 1);
    u16 index = readTexture(byteX, ty + by);
    u16 entry = index >> ((byteX & 1) * 8) >> ((tx & 1) * 4) & 15;
    return readCLUT(entry);
  }

  if(textureDepth == 1) {  //8bpp
    u32 byteX = tx + (bx << 1);
    u16 index = readTexture(byteX, ty + by);
    u16 entry = index >> (byteX & 1) * 8 & 255;
    return readCLUT(entry);
  }

  if(textureDepth >= 2) {
    return readTexture((tx + bx) << 1, ty + by);
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

auto GPU::Render::modulate(Color texel, Color color) const -> Color {
  texel.r = min(255, (color.r * (texel.r & 0xf8)) >> 7);
  texel.g = min(255, (color.g * (texel.g & 0xf8)) >> 7);
  texel.b = min(255, (color.b * (texel.b & 0xf8)) >> 7);

  return texel;
}

auto GPU::Render::alpha(u16 above, u16 below) const -> u16 {
  switch(semiTransparency) {
  case 0: {
    below |= 0x8000;
    return ((above + below) - ((above ^ below) & 0x0421)) >> 1;
  }
  case 1: {
    below &= ~0x8000;
    u32 sum = above + below;
    u32 carry = (sum - ((above ^ below) & 0x8421)) & 0x8420;
    return (sum - carry) | (carry - (carry >> 5));
  }
  case 2: {
    below |= 0x8000;
    above &= ~0x8000;
    u32 diff = below - above + 0x108420;
    u32 borrow = (diff - ((below ^ above) & 0x108420)) & 0x108420;
    return (diff - borrow) & (borrow - (borrow >> 5));
  }
  case 3: {
    above = ((above >> 2) & 0x1ce7) | 0x8000;
    below &= ~0x8000;
    u32 sum = above + below;
    u32 carry = (sum - ((above ^ below) & 0x8421)) & 0x8420;
    return (sum - carry) | (carry - (carry >> 5));
  }
  }

  return above;
}

template<u32 Flags>
auto GPU::Render::pixel(Point point, Color rgb, Point uv, u16* destination) -> void {
  if(interlacedRendering && (point.y & 1) == activeLineLSB) return;

  Color above;
  bool transparent;
  bool maskBit = forceMaskBit;

  if constexpr(Flags & Texture) {
    u16 pixel = texel(uv);
    if(!pixel) return;

    transparent = pixel >> 15;
    if constexpr((Flags & Alpha) && (Flags & Rectangle)) {
      if(transparent) gpu.renderer.textureBlendPixels++;
    }
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

  if(!destination) destination = &gpu.vram2D[point.y & 511][point.x & 1023];
  u16 input = *destination;
  u16 output = above.to16();

  if constexpr(Flags & Alpha) {
    if(transparent) output = alpha(output | 0x8000, input);
  }
  if constexpr(!(Flags & Texture)) {
    output &= 0x7fff;
  }

  if(input >> 15 & checkMaskBit) return;
  *destination = output | maskBit << 15;
}

template<typename Draw>
auto GPU::Render::writeSpan(s32 x, s32 y, s32 width, Draw&& draw) -> void {
  u16 buffer[18];
  s32 pending = 0;
  s32 written = 0;
  s32 rendered = 0;
  s32 batch = 17;
  const s32 tail = (x & 1) ? 2 : 1;
  while(rendered < width) {
    s32 count = min(batch, width - rendered);
    for(s32 i = 0; i < count; i++) {
      buffer[pending + i] = gpu.vram2D[y & 511][(x + rendered + i) & 1023];
    }
    for(s32 i = 0; i < count; i++) {
      draw(x + rendered + i, &buffer[pending + i]);
    }
    rendered += count;
    pending += count;
    s32 drain = rendered == width ? pending : pending - tail;
    for(s32 i = 0; i < drain; i++) {
      gpu.vram2D[y & 511][(x + written++) & 1023] = buffer[i];
    }
    pending -= drain;
    for(s32 i = 0; i < pending; i++) buffer[i] = buffer[drain + i];
    batch = 16;
  }
}

template<u32 Flags>
auto GPU::Render::line() -> void {
  if(v0.x > v1.x || (v0.x == v1.x && v0.y != v1.y)) swap(v0, v1);

  Point d = {v1.x - v0.x, v1.y - v0.y};
  if(abs(d.x) >= 1024 || abs(d.y) >= 512) return;

  s32 steps = abs(d.x) > abs(d.y) ? abs(d.x) : abs(d.y);
  if(steps == 0) {
    Point point{i11(v0.x + drawingAreaOffsetX), i11(v0.y + drawingAreaOffsetY)};
    if(point.x >= drawingAreaOriginX1 && point.x <= drawingAreaOriginX2
    && point.y >= drawingAreaOriginY1 && point.y <= drawingAreaOriginY2) {
      return pixel<Flags | Dither>(point, v0);
    }
    return;
  }

  static constexpr s32 VertexBits = 32;
  static constexpr s64 VertexHalf = 1ll << (VertexBits - 1);
  static constexpr s64 VertexBias = 1024;
  static constexpr s64 VertexFixedMask = (1ll << (11 + VertexBits)) - 1;
  static constexpr s64 VertexFixedSign = 1ll << (10 + VertexBits);

  auto divide = [](s64 delta, s32 steps) -> s64 {
    return delta / steps + (delta % steps ? (delta > 0 ? 1 : -1) : 0);
  };

  s64 dx = divide(s64(d.x) << VertexBits, steps);
  s64 dy = divide(s64(d.y) << VertexBits, steps);
  s64 x = (s64(v0.x) << VertexBits) + VertexHalf - VertexBias;
  s64 y = (s64(v0.y) << VertexBits) + VertexHalf - (dy < 0 ? VertexBias : 0);

  x += s64(drawingAreaOffsetX) << VertexBits;
  y += s64(drawingAreaOffsetY) << VertexBits;

  auto clipFixed = [](s64 value) -> s64 {
    value &= VertexFixedMask;
    if(value & VertexFixedSign) value |= ~VertexFixedMask;
    return value;
  };
  x = clipFixed(x);
  y = clipFixed(y);

  s32 r = (v0.r << 12) | (1 << 11);
  s32 g = (v0.g << 12) | (1 << 11);
  s32 b = (v0.b << 12) | (1 << 11);
  s32 dr = 0;
  s32 dg = 0;
  s32 db = 0;
  if constexpr(Flags & Shade) {
    dr = ((v1.r - v0.r) << 12) / steps;
    dg = ((v1.g - v0.g) << 12) / steps;
    db = ((v1.b - v0.b) << 12) / steps;
  }

  u32 pixels = 0;
  for(u32 step : range((u32)steps + 1)) {
    Point point{s32(x >> VertexBits), s32(y >> VertexBits)};
    if(point.x >= drawingAreaOriginX1 && point.x <= drawingAreaOriginX2
    && point.y >= drawingAreaOriginY1 && point.y <= drawingAreaOriginY2) {
      pixel<Flags | Dither>(point, Color::fromRGB(r >> 12, g >> 12, b >> 12));
    }
    x += dx, y += dy;
    if constexpr(Flags & Shade) r += dr, g += dg, b += db;
    pixels++;
  }

//io.pcounter += cost<Flags | Line>(pixels);
}

template<u32 Flags>
auto GPU::Render::triangle() -> void {
  static constexpr u32 Dithering = ((
    ((Flags & Shade) && !((Flags & Texture) && (Flags & Raw)))
    || ((Flags & Texture) && !(Flags & Raw))
  ) && !(Flags & Rectangle)) ? Dither : 0;
  static constexpr s32 EdgeBits = 20;
  static constexpr s64 EdgeFraction = (1ll << EdgeBits) - 1;

  if constexpr(Flags & Texture) loadCLUTCache();

  auto g0 = v0;
  auto g1 = v1;
  auto g2 = v2;

  s32 signedArea = drawableArea(g0, g1, g2);
  s32 area = signedArea;
  if(area == 0) return;  //do not render colinear triangles
  if(area < 0) swap(g1, g2), area = -area;

  Delta dr{}, dg{}, db{}, du{}, dv{};
  s64 rBase = s64(g0.r) << 12;
  s64 gBase = s64(g0.g) << 12;
  s64 bBase = s64(g0.b) << 12;
  s64 uBase = 0;
  s64 vBase = 0;

  if constexpr(Flags & Shade) {
    s32 r[3] = {g0.r, g1.r, g2.r};
    s32 g[3] = {g0.g, g1.g, g2.g};
    s32 b[3] = {g0.b, g1.b, g2.b};
    dr = delta(g0, g1, g2, r, area);
    dg = delta(g0, g1, g2, g, area);
    db = delta(g0, g1, g2, b, area);
  }

  if constexpr(Flags & Texture) {
    s32 u[3] = {g0.u, g1.u, g2.u};
    s32 v[3] = {g0.v, g1.v, g2.v};
    du = delta(g0, g1, g2, u, area);
    dv = delta(g0, g1, g2, v, area);
  }

  auto xs0 = v0, xs1 = v1, xs2 = v2;
  if(xs1.y < xs0.y) swap(xs0, xs1);
  if(xs2.y < xs1.y) swap(xs1, xs2);
  if(xs1.y < xs0.y) swap(xs0, xs1);
  if(signedArea < 0) swap(xs0, xs2);
  if(xs1.x < xs0.x) swap(xs0, xs1);
  if(xs2.x < xs1.x) swap(xs1, xs2);
  if(xs1.x < xs0.x) swap(xs0, xs1);

  if constexpr(Flags & Shade) {
    rBase = (s64(xs0.r) << 12) + (1 << 11);
    gBase = (s64(xs0.g) << 12) + (1 << 11);
    bBase = (s64(xs0.b) << 12) + (1 << 11);
  }
  if constexpr(Flags & Texture) {
    uBase = (s64(xs0.u) << 12) + (1 << 11);
    vBase = (s64(xs0.v) << 12) + (1 << 11);
  }

  struct Edge {
    s32 y0 = 0;
    s32 y1 = 0;
    s64 x = 0;
    s64 step = 0;
    bool active = false;
  };

  auto makeEdge = [&](Vertex from, Vertex to) -> Edge {
    Edge edge;
    if(from.y == to.y) return edge;
    s32 height = abs(to.y - from.y);
    edge.y0 = from.y;
    edge.y1 = to.y;
    edge.x = (s64(from.x) << EdgeBits) | EdgeFraction;
    edge.step = (s64(to.x - from.x) << EdgeBits) / height;
    if(edge.step < 0) edge.step--;
    if(to.y < from.y) edge.step = -edge.step;
    edge.active = true;
    return edge;
  };

  auto edgeXAt = [&](Edge edge, s32 y) -> s64 {
    return edge.x + edge.step * (y - edge.y0);
  };

  auto drawSpan = [&](s32 y, s32 xStart, s32 xEnd) -> u32 {
    if(xEnd <= xStart) return 0;
    u32 pixels = 0;
    s32 screenX = i11(xStart + drawingAreaOffsetX);
    s32 screenY = i11(xs0.y + drawingAreaOffsetY) + (y - xs0.y);
    s32 baseX = i11(xs0.x + drawingAreaOffsetX);
    s32 baseY = i11(xs0.y + drawingAreaOffsetY);
    s64 rPoint = rBase + s64(dr.x) * (screenX - baseX) + s64(dr.y) * (screenY - baseY);
    s64 gPoint = gBase + s64(dg.x) * (screenX - baseX) + s64(dg.y) * (screenY - baseY);
    s64 bPoint = bBase + s64(db.x) * (screenX - baseX) + s64(db.y) * (screenY - baseY);
    s64 uPoint = uBase + s64(du.x) * (screenX - baseX) + s64(du.y) * (screenY - baseY);
    s64 vPoint = vBase + s64(dv.x) * (screenX - baseX) + s64(dv.y) * (screenY - baseY);

    s32 skip = max(0, drawingAreaOriginX1 - screenX);
    s32 width = min(xEnd - xStart - skip, drawingAreaOriginX2 + 1 - (screenX + skip));
    if(width <= 0 || screenY < drawingAreaOriginY1 || screenY > drawingAreaOriginY2) return 0;
    screenX += skip;
    rPoint += s64(dr.x) * skip, gPoint += s64(dg.x) * skip, bPoint += s64(db.x) * skip;
    uPoint += s64(du.x) * skip, vPoint += s64(dv.x) * skip;
    auto draw = [&](s32 x, u16* destination) {
      screenX = x;
      Point out{screenX, screenY};
      if(out.x >= drawingAreaOriginX1 && out.x <= drawingAreaOriginX2
      && out.y >= drawingAreaOriginY1 && out.y <= drawingAreaOriginY2) {
        Color c = Color::fromRGB(
          uclamp<8>(rPoint >> 12),
          uclamp<8>(gPoint >> 12),
          uclamp<8>(bPoint >> 12)
        );
        if constexpr(Flags & Texture) {
          pixel<Flags | Dithering>(out, c, {s32(uPoint >> 12), s32(vPoint >> 12)}, destination);
        } else {
          pixel<Flags | Dithering>(out, c, {}, destination);
        }
        pixels++;
      }

      rPoint += dr.x, gPoint += dg.x, bPoint += db.x;
      if constexpr(Flags & Texture) uPoint += du.x, vPoint += dv.x;
    };
    if constexpr(Flags & Texture) {
      writeSpan(screenX, screenY, width, draw);
    } else {
      s32 first = screenX;
      for(s32 x = first; x < first + width; x++) draw(x, nullptr);
    }
    return pixels;
  };

  auto drawPart = [&](s32 yStart, s32 yEnd, Edge left, Edge right) -> u32 {
    if(yEnd == yStart) return 0;
    u32 pixels = 0;
    bool up = yEnd < yStart;
    s32 step = up ? -1 : 1;
    for(s32 row = yStart; row != yEnd; row += step) {
      s32 y = row - s32(up);
      s64 xa = edgeXAt(left, y);
      s64 xb = edgeXAt(right, y);
      if(xa < xb) {
        pixels += drawSpan(y, s32(xa >> EdgeBits), s32(xb >> EdgeBits));
      }
    }
    return pixels;
  };

  u32 pixels = 0;
  auto t0 = g0, t1 = g1, t2 = g2;
  if(t1.y < t0.y) swap(t0, t1);
  if(t2.y < t1.y) swap(t1, t2);
  if(t1.y < t0.y) swap(t0, t1);

  if(t0.y != t2.y) {
    bool minorLeft = weight(t0, t1, t2) < 0;
    auto part = [&](s32 start, s32 end, Edge minor, Edge major) {
      return minorLeft ? drawPart(start, end, minor, major) : drawPart(start, end, major, minor);
    };
    if(xs0.y == t0.y) {
      auto major = makeEdge(t0, t2);
      pixels += part(t0.y, t1.y, makeEdge(t0, t1), major);
      pixels += part(t1.y, t2.y, makeEdge(t1, t2), major);
    } else if(xs0.y == t2.y) {
      auto major = makeEdge(t2, t0);
      pixels += part(t2.y, t1.y, makeEdge(t2, t1), major);
      pixels += part(t1.y, t0.y, makeEdge(t1, t0), major);
    } else {
      auto major = makeEdge(t0, t2);
      pixels += part(t1.y, t2.y, makeEdge(t1, t2), major);
      pixels += part(t1.y, t0.y, makeEdge(t1, t0), major);
    }
  }

//io.pcounter += cost<Flags>(pixels);
}

template<u32 Flags>
auto GPU::Render::quadrilateral() -> void {
  auto q0 = v0, q1 = v1, q2 = v2, q3 = v3;
  v0 = q0, v1 = q1, v2 = q2;
  triangle<Flags>();
  v0 = q1, v1 = q2, v2 = q3;
  triangle<Flags>();

  v0 = q0, v1 = q1, v2 = q2, v3 = q3;
}

template<u32 Flags>
auto GPU::Render::rectangle() -> void {
  if constexpr(Flags & Texture) loadCLUTCache();

  s32 x0 = i11(v0.x + drawingAreaOffsetX);
  s32 y0 = i11(v0.y + drawingAreaOffsetY);
  s32 x1 = x0 + size.w;
  s32 y1 = y0 + size.h;
  if(size.w == 0 || size.h == 0) return;
  if(x1 <= drawingAreaOriginX1 || y1 <= drawingAreaOriginY1) return;
  if(x0 > drawingAreaOriginX2 || y0 > drawingAreaOriginY2) return;

  s32 u = v0.u;
  s32 v = v0.v;
  s32 uinc = 1;
  s32 vinc = 1;

  if constexpr(Flags & Texture) {
    texelErrorMask = !textureFlipX && (v0.u & 1) ? (textureDepth < 2 ? 0x07 : 0x03) : 0;
    if(textureFlipX) {
      u = v0.u | 1;
      uinc = -1;
    }
    if(textureFlipY) {
      vinc = -1;
    }
  }

  s32 xStart = max(x0, drawingAreaOriginX1);
  s32 yStart = max(y0, drawingAreaOriginY1);
  s32 xEnd = min(x1, drawingAreaOriginX2 + 1);
  s32 yEnd = min(y1, drawingAreaOriginY2 + 1);
  if constexpr(Flags & Texture) {
    u += (xStart - x0) * uinc;
    v += (yStart - y0) * vinc;
  }

  u32 pixels = 0;
  for(s32 y : range(yStart, yEnd)) {
    s32 uLine = u;
    if constexpr(Flags & Texture) {
      auto draw = [&](s32 x, u16* destination) {
        pixel<Flags | Rectangle>({x, y}, v0, {u8(uLine), u8(v)}, destination);
        uLine += uinc;
        pixels++;
      };
      writeSpan(xStart, y, xEnd - xStart, draw);
    } else {
      for(s32 x : range(xStart, xEnd)) {
        pixel<Flags | Rectangle>({x, y}, v0, {}, nullptr);
        pixels++;
      }
    }
    if constexpr(Flags & Texture) v += vinc;
  }

  if constexpr(Flags & Texture) {
    texelErrorMask = 0;
  }

}

template<u32 Flags>
auto GPU::Render::fill() -> void {
  auto color = v0.to16();
  for(u32 y : range(size.h)) {
    if(interlacedRendering && (((y + v0.y) & 1) == activeLineLSB)) continue;
    for(u32 x : range(size.w)) {
      u32 vy = (y + v0.y) & 511;
      u32 vx = (x + v0.x) & 1023;
      gpu.vram2D[vy][vx] = color;
    }
  }
//io.pcounter += cost<Flags>(size.width * size.height);
}

static constexpr double FittedWidth = 320.0;
static constexpr double RectScanline = 5.00 * CpuToGpuClocks;
static constexpr double PolyScanline = 4.75 * CpuToGpuClocks;

auto GPU::drawCycles(const Render& render) const -> u32 {
  u32 command = render.command;
  bool semi = command & 2;
  bool textured = command & 4;
  double perPixel = 0.0;
  double perScanline = 0.0;
  u32 pixels = 0;
  u32 scanlines = 0;

  if(command == 0x02) {
    pixels = render.size.w * render.size.h;
    perPixel = 0.137;
  } else if(command >= 0x20 && command <= 0x3f) {
    s32 first = render.drawableArea(render.v0, render.v1, render.v2);
    s32 second = command & 8 ? render.drawableArea(render.v1, render.v2, render.v3) : 0;
    if(!first && !second) return 0;
    pixels = (u32)abs(first) >> 1;
    if(second) pixels += (u32)abs(second) >> 1;
    s32 ox = render.drawingAreaOffsetX, oy = render.drawingAreaOffsetY;
    Point origin = first ? render.v0 : render.v1;
    s32 x0 = origin.x + ox, x1 = x0, y0 = origin.y + oy, y1 = y0;
    auto extend = [&](const Vertex& v) {
      x0 = min(x0, v.x + ox); x1 = max(x1, v.x + ox);
      y0 = min(y0, v.y + oy); y1 = max(y1, v.y + oy);
    };
    extend(render.v1); extend(render.v2);
    if(second) extend(render.v3);
    s64 full = (s64)(x1 - x0 + 1) * (y1 - y0 + 1);
    s32 cx0 = max(x0, render.drawingAreaOriginX1), cx1 = min(x1, render.drawingAreaOriginX2);
    s32 cy0 = max(y0, render.drawingAreaOriginY1), cy1 = min(y1, render.drawingAreaOriginY2);
    s64 clipped = (s64)max(0, cx1 - cx0 + 1) * max(0, cy1 - cy0 + 1);
    if(!clipped) return 0;
    if(full > 0) pixels = (u32)((s64)pixels * clipped / full);
    scanlines = max(0, min(cy1, y1) - max(cy0, y0) + 1);
    perScanline = PolyScanline;
    perPixel = (textured || (command & 0x10) ? 1.746 : (semi ? 1.314 : 0.873)) - PolyScanline / FittedWidth;
  } else if(command >= 0x40 && command <= 0x5f) {
    s32 dx = render.v1.x - render.v0.x, dy = render.v1.y - render.v0.y;
    if(abs(dx) >= 1024 || abs(dy) >= 512) return 0;
    s32 x0 = i11(render.v0.x + render.drawingAreaOffsetX);
    s32 y0 = i11(render.v0.y + render.drawingAreaOffsetY);
    s32 x1 = i11(render.v1.x + render.drawingAreaOffsetX);
    s32 y1 = i11(render.v1.y + render.drawingAreaOffsetY);
    if(max(x0, x1) < render.drawingAreaOriginX1 || min(x0, x1) > render.drawingAreaOriginX2
    || max(y0, y1) < render.drawingAreaOriginY1 || min(y0, y1) > render.drawingAreaOriginY2) return 0;
    pixels = max(dx < 0 ? -dx : dx, dy < 0 ? -dy : dy);
    perPixel = 1.0;
  } else if(command >= 0x60 && command <= 0x7f) {
    s32 x0 = i11(render.v0.x + render.drawingAreaOffsetX);
    s32 y0 = i11(render.v0.y + render.drawingAreaOffsetY);
    s32 x1 = x0 + render.size.w;
    s32 y1 = y0 + render.size.h;
    u32 width = max(0, min(x1, render.drawingAreaOriginX2 + 1) - max(x0, render.drawingAreaOriginX1));
    u32 height = max(0, min(y1, render.drawingAreaOriginY2 + 1) - max(y0, render.drawingAreaOriginY1));
    if(!width || !height) return 0;
    pixels = width * height;
    scanlines = height;
    perScanline = RectScanline;
    perPixel = (!textured && semi ? 1.298 : 0.858) - RectScanline / FittedWidth;
  } else {
    return 0;
  }

  if(render.interlacedRendering && ((command >= 0x20 && command <= 0x3f)
                               || (command >= 0x60 && command <= 0x7f))) {
    pixels /= 2;
    if(scanlines) scanlines = max(1u, scanlines / 2);
  }

  return (u32)(pixels * perPixel + scanlines * perScanline);
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
  case 0x102: return gpu.invalidateTextureCache();
  case 0x103: return gpu.invalidateCache();
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
    workSequence.fetch_add(1, std::memory_order_release);
    workSequence.notify_one();
  } else if constexpr(true) {
    render.execute();
  }
}

auto GPU::Renderer::synchronize() -> void {
  if constexpr(Accuracy::GPU::Threaded) {
    u32 target = workSequence.load(std::memory_order_acquire);
    while(completedSequence.load(std::memory_order_acquire) != target) spinloop();
  }
}

auto GPU::Renderer::main(uintptr_t) -> void {
  while(true) {
    for(u32 spins = 0; spins < 1024 && fifo.empty(); spins++) spinloop();
    while(fifo.empty()) {
      u32 sequence = workSequence.load(std::memory_order_acquire);
      if(fifo.empty()) workSequence.wait(sequence, std::memory_order_acquire);
    }
    auto render = fifo.await_read();
    self.vram.mutex.lock();
    render.execute();
    self.vram.mutex.unlock();
    completedSequence.fetch_add(1, std::memory_order_release);
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
    workSequence = 0;
    completedSequence = 0;
    handle = thread::create(std::bind_front(&GPU::Renderer::main, &self.renderer));
  }
}
