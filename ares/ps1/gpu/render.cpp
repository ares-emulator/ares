//compute barycentric weight (area) between three vertices
auto GPU::weight(Point a, Point b, Point c) const -> i32 {
  return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

auto GPU::texel(Point p) const -> u16 {
  u16 px = io.texturePaletteX << 4;
  u16 py = io.texturePaletteY << 0;
  i32 bx = io.texturePageBaseX << 6;
  i32 by = io.texturePageBaseY << 8;
  i32 tx = p.x & ~(io.textureWindowMaskX * 8) | (io.textureWindowOffsetX & io.textureWindowMaskX) * 8;
  i32 ty = p.y & ~(io.textureWindowMaskY * 8) | (io.textureWindowOffsetY & io.textureWindowMaskY) * 8;

  if(io.textureDepth == 0) {  //4bpp
    i32 ix = tx / 4 + bx & 1023;
    i32 iy = ty + by & 511;
    u16 index = vram.readHalf((iy * 1024 + ix) * 2);
    u16 entry = index >> (tx & 3) * 4 & 15;
    i32 cx = px + entry & 1023;
    i32 cy = py;
    return vram.readHalf((cy * 1024 + cx) * 2);
  }

  if(io.textureDepth == 1) {  //8bpp
    i32 ix = tx / 2 + bx & 1023;
    i32 iy = ty + by & 511;
    u16 index = vram.readHalf((iy * 1024 + ix) * 2);
    u16 entry = index >> (tx & 1) * 8 & 255;
    i32 cx = px + entry & 1023;
    i32 cy = py;
    return vram.readHalf((cy * 1024 + cx) * 2);
  }

  if(io.textureDepth == 2) {  //16bpp
    i32 ix = tx + bx & 1023;
    i32 iy = ty + by & 511;
    return vram.readHalf((iy * 1024 + ix) * 2);
  }

  return 0;
}

auto GPU::dither(Point p, Color c) const -> Color {
  if(io.dithering) {
    static constexpr i8 table[4 * 4] = {
      -4, +0, -3, +1,
      +2, -2, +3, -1,
      -3, +1, -4, +0,
      +3, -1, +2, -2,
    };
    i8 d = table[(p.y & 3) * 4 + (p.x & 3)];
    c.r = max(0, min(255, (i16)c.r + d));
    c.g = max(0, min(255, (i16)c.g + d));
    c.b = max(0, min(255, (i16)c.b + d));
  }
  return c;
}

auto GPU::renderPixelColor(Point point, Color color) -> void {
  point.x = point.x + io.drawingAreaOffsetX;
  point.y = point.y + io.drawingAreaOffsetY;

  if(point.x < io.drawingAreaOriginX1) return;
  if(point.y < io.drawingAreaOriginY1) return;

  if(point.x > io.drawingAreaOriginX2) return;
  if(point.y > io.drawingAreaOriginY2) return;

  u32 address = (point.y & 511) * 1024 + (point.x & 1023);
  vram.writeHalf(address * 2, dither(point, color).to16());
}

auto GPU::renderPixelAlpha(Point point, Color above) -> void {
  point.x = point.x + io.drawingAreaOffsetX;
  point.y = point.y + io.drawingAreaOffsetY;

  if(point.x < io.drawingAreaOriginX1) return;
  if(point.y < io.drawingAreaOriginY1) return;

  if(point.x > io.drawingAreaOriginX2) return;
  if(point.y > io.drawingAreaOriginY2) return;

  u32 address = (point.y & 511) * 1024 + (point.x & 1023);
  u16 data = vram.readHalf(address * 2);

  if(data & 0x8000) {
    Color below = Color::from16(data);
    switch(io.semiTransparency) {
    case 0:
      above.r = (below.r >> 1) + (above.r >> 1);
      above.g = (below.g >> 1) + (above.g >> 1);
      above.b = (below.b >> 1) + (above.b >> 1);
      break;
    case 1:
      above.r = min(255, (u16)below.r + (u16)above.r);
      above.g = min(255, (u16)below.g + (u16)above.g);
      above.b = min(255, (u16)below.b + (u16)above.b);
      break;
    case 2:
      above.r = max(  0, (i16)below.r - (i16)above.r);
      above.g = max(  0, (i16)below.g - (i16)above.g);
      above.b = max(  0, (i16)below.b - (i16)above.b);
      break;
    case 3:
      above.r = min(255, (u16)below.r + (above.r >> 2));
      above.g = min(255, (u16)below.g + (above.g >> 2));
      above.b = min(255, (u16)below.b + (above.b >> 2));
      break;
    }
  }

  vram.writeHalf(address * 2, dither(point, above).to16());
}

auto GPU::renderSolidLine(Point p0, Point p1, Color c) -> void {
  Point d = {p1.x - p0.x, p1.y - p0.y};
  i32 steps = abs(d.x) > abs(d.y) ? abs(d.x) : abs(d.y);
  Point s = {(d.x << 16) / steps, (d.y << 16) / steps};
  Point p = {p0.x << 16, p0.y << 16};

  for(u16 step : range(steps)) {
    renderPixelColor({p.x >> 16, p.y >> 16}, c);
    p.x += s.x;
    p.y += s.y;
  }
}

//#define RENDER_DDA
#define RENDER_BARYCENTRIC

#if defined(RENDER_DDA)
template<uint Flags>
auto GPU::renderTriangle(Vertex v0, Vertex v1, Vertex v2) -> void {
  //sort: y0 <= y1 <= y2
  if(v0.y > v1.y) swap(v0, v1);
  if(v0.y > v2.y) swap(v0, v2);
  if(v1.y > v2.y) swap(v1, v2);

  struct Slope { i32 l, r; };
  Slope xy{v0.x << 16, v0.x << 16}, xs;
  Slope ry{v0.r << 16, v0.r << 16}, rs;
  Slope gy{v0.g << 16, v0.g << 16}, gs;
  Slope by{v0.b << 16, v0.b << 16}, bs;
  Slope uy{v0.u << 16, v0.u << 16}, us;
  Slope vy{v0.v << 16, v0.v << 16}, vs;

  if(v0.y == v1.y) {
    xy.r = v1.x << 16, ry.r = v1.r << 16, gy.r = v1.g << 16, by.r = v1.b << 16, uy.r = v1.u << 16, vy.r = v1.v << 16;
    if(v0.x > v1.x) swap(xy.l, xy.r), swap(ry.l, ry.r), swap(gy.l, gy.r), swap(by.l, by.r), swap(uy.l, uy.r), swap(vy.l, vy.r);
  }

  for(i32 y = v0.y; y < v2.y; y++) {
    #define interpolate(f) {(v1.f - v0.f << 16) / (v1.y - v0.y), (v2.f - v0.f << 16) / (v2.y - v0.y)}
    if(y == v0.y && v0.y < v1.y) {
      xs = interpolate(x);
      if constexpr(Flags & Render::Shade) rs = interpolate(r), gs = interpolate(g), bs = interpolate(b);
      if constexpr(Flags & Render::Texel) us = interpolate(u), vs = interpolate(v);
      if(v1.x > v2.x) {
        swap(xs.l, xs.r);
        if constexpr(Flags & Render::Shade) swap(rs.l, rs.r), swap(gs.l, gs.r), swap(bs.l, bs.r);
        if constexpr(Flags & Render::Texel) swap(us.l, us.r), swap(vs.l, vs.r);
      }
    }
    #undef interpolate

    #define interpolate(f) {(v2.f - v0.f << 16) / (v2.y - v0.y), (v2.f - v1.f << 16) / (v2.y - v1.y)}
    if(y == v1.y && v1.y < v2.y) {
      xs = interpolate(x);
      if constexpr(Flags & Render::Shade) rs = interpolate(r), gs = interpolate(g), bs = interpolate(b);
      if constexpr(Flags & Render::Texel) us = interpolate(u), vs = interpolate(v);
      if(v1.x <= v2.x) {
        swap(xs.l, xs.r);
        if constexpr(Flags & Render::Shade) swap(rs.l, rs.r), swap(gs.l, gs.r), swap(bs.l, bs.r);
        if constexpr(Flags & Render::Texel) swap(us.l, us.r), swap(vs.l, vs.r);
      }
    }
    #undef interpolate

    #define interpolate(f) (xy.r - xy.l >> 16) ? (f.r - f.l) / (xy.r - xy.l >> 16) : 0
    i32 rx, gx, bx, ux, vx;
    if constexpr(Flags & Render::Shade) rx = interpolate(ry), gx = interpolate(gy), bx = interpolate(by);
    if constexpr(Flags & Render::Texel) ux = interpolate(uy), vx = interpolate(vy);
    #undef interpolate
    i32 r = ry.l, g = gy.l, b = by.l, u = uy.l, v = vy.l;

    for(i32 x = xy.l >> 16; x < xy.r >> 16; x++) {
      if constexpr(Flags & Render::Color) {
        renderPixelColor({x, y}, v0);
      }
      if constexpr(Flags & Render::Shade) {
        renderPixelColor({x, y}, {r >> 16, g >> 16, b >> 16});
        r += rx, g += gx, b += bx;
      }
      if constexpr(Flags & Render::Texel) {
        if(u16 texel = this->texel({u >> 16, v >> 16})) {
          renderPixelColor({x, y}, Color::from16(texel));
        }
        u += ux, v += vx;
      }
    }

    xy.l += xs.l, xy.r += xs.r;
    if constexpr(Flags & Render::Shade) {
      ry.l += rs.l, ry.r += rs.r;
      gy.l += gs.l, gy.r += gs.r;
      by.l += bs.l, by.r += bs.r;
    }
    if constexpr(Flags & Render::Texel) {
      uy.l += us.l, uy.r += us.r;
      vy.l += vs.l, vy.r += vs.r;
    }
  }
}
#endif

#if defined(RENDER_BARYCENTRIC)
template<uint Flags>
auto GPU::renderTriangle(Vertex v0, Vertex v1, Vertex v2) -> void {
  Point vmin{min(v0.x, v1.x, v2.x), min(v0.y, v1.y, v2.y)};
  Point vmax{max(v0.x, v1.x, v2.x), max(v0.y, v1.y, v2.y)};

  i32 w = weight(v0, v1, v2);  //<0 = counter-clockwise; 0 = colinear, >0 = clockwise
  if(w < 0) swap(v1, v2), w = -w;  //make clockwise

  bool o0 = (v2.y - v1.y == 0 && v2.x - v1.x > 0) || (v2.y - v1.y < 0);
  bool o1 = (v0.y - v2.y == 0 && v0.x - v2.x > 0) || (v0.y - v2.y < 0);
  bool o2 = (v1.y - v0.y == 0 && v1.x - v0.x > 0) || (v1.y - v0.y < 0);

  Point vp;
  for(vp.y = vmin.y; vp.y <= vmax.y; vp.y++) {
    for(vp.x = vmin.x; vp.x <= vmax.x; vp.x++) {
      i32 w0 = weight(v1, v2, vp);
      i32 w1 = weight(v2, v0, vp);
      i32 w2 = weight(v0, v1, vp);

      if(w0 < 0 || !w0 && !o0) continue;
      if(w1 < 0 || !w1 && !o1) continue;
      if(w2 < 0 || !w2 && !o2) continue;

      if constexpr(Flags & Render::Color) {
        renderPixelColor(vp, v0);
      }

      if constexpr(Flags & Render::Shade) {
        u8 r = (v0.r * w0 + v1.r * w1 + v2.r * w2) / w;
        u8 g = (v0.g * w0 + v1.g * w1 + v2.g * w2) / w;
        u8 b = (v0.b * w0 + v1.b * w1 + v2.b * w2) / w;
        renderPixelColor(vp, {r, g, b});
      }

      if constexpr(Flags & Render::Texel) {
        u8 u = (v0.u * w0 + v1.u * w1 + v2.u * w2) / w;
        u8 v = (v0.v * w0 + v1.v * w1 + v2.v * w2) / w;
        if(u16 texel = this->texel({u, v})) {
          Color c = Color::from16(texel);
          if constexpr(Flags & Render::ModulateColor) {
            c.r = c.r * v0.r >> 7;
            c.g = c.g * v0.g >> 7;
            c.b = c.b * v0.b >> 7;
          }
          if constexpr(Flags & Render::ModulateShade) {
            u8 r = (v0.r * w0 + v1.r * w1 + v2.r * w2) / w;
            u8 g = (v0.g * w0 + v1.g * w1 + v2.g * w2) / w;
            u8 b = (v0.b * w0 + v1.b * w1 + v2.b * w2) / w;
            c.r = c.r * r >> 7;
            c.g = c.g * g >> 7;
            c.b = c.b * b >> 7;
          }
          if constexpr(Flags & Render::Alpha) {
            renderPixelAlpha(vp, c);
          } else {
            renderPixelColor(vp, c);
          }
        }
      }
    }
  }
}
#endif

template<uint Flags>
auto GPU::renderQuadrilateral(Vertex v0, Vertex v1, Vertex v2, Vertex v3) -> void {
  renderTriangle<Flags>(v0, v1, v2);
  renderTriangle<Flags>(v1, v2, v3);
}

template<uint Flags>
auto GPU::renderRectangle(Vertex v0, Size sz) -> void {
  auto v1 = Vertex(v0).setPoint(v0.x + sz.w, v0.y).setTexel(v0.u + sz.w, v0.v);
  auto v2 = Vertex(v0).setPoint(v0.x, v0.y + sz.h).setTexel(v0.u, v0.v + sz.h);
  auto v3 = Vertex(v0).setPoint(v0.x + sz.w, v0.y + sz.h).setTexel(v0.u + sz.w, v0.v + sz.h);
  renderQuadrilateral<Flags>(v0, v1, v2, v3);
}
