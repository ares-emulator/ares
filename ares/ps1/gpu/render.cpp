//compute barycentric weight between three vertices
auto GPU::weight(Point a, Point b, Point c) const -> i32 {
  return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

auto GPU::renderPixel(Point p, Color c) -> void {
  u32 address = (p.y & 511) * 1024 + (p.x & 1023);
  u16 pixel = (c.r >> 3) << 10 | (c.g >> 3) << 5 | (c.b >> 3) << 0;
  vram.writeHalf(address * 2, pixel);
}

auto GPU::renderSolidLine(Point p0, Point p1, Color c) -> void {
  double dx = p1.x - p0.x;
  double dy = p1.y - p0.y;
  double steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
  double xs = dx / steps;
  double ys = dy / steps;
  double x = p0.x;
  double y = p0.y;
  for(uint i = 0; i <= uint(steps); i++) {
    renderPixel({uint(x), uint(y)}, c);
    x += xs;
    y += ys;
  }

  #if 0
  Point d = {p1.x - p0.x, p1.y - p0.y};
  u32 steps = abs(d.x) > abs(d.y) ? abs(d.x) : abs(d.y);
  Point s = {(d.x << 16) / steps, (d.y << 16) / steps};
  Point p = {p0.x << 16, p0.y << 16};

  for(u16 step : range(steps)) {
    renderPixel({p.x >> 16, p.y >> 16}, c);
    p.x += s.x;
    p.y += s.y;
  }
  #endif
}

#define RENDER_DDA
//#define RENDER_BARYCENTRIC

#if defined(RENDER_DDA)
template<uint Flags>
auto GPU::renderTriangle(Vertex v0, Vertex v1, Vertex v2, Texture ts) -> void {
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
      if constexpr(Flags & Render::Color) rs = interpolate(r), gs = interpolate(g), bs = interpolate(b);
      if constexpr(Flags & Render::Texel) us = interpolate(u), vs = interpolate(v);
      if(v1.x > v2.x) {
        swap(xs.l, xs.r);
        if constexpr(Flags & Render::Color) swap(rs.l, rs.r), swap(gs.l, gs.r), swap(bs.l, bs.r);
        if constexpr(Flags & Render::Texel) swap(us.l, us.r), swap(vs.l, vs.r);
      }
    }
    #undef interpolate

    #define interpolate(f) {(v2.f - v0.f << 16) / (v2.y - v0.y), (v2.f - v1.f << 16) / (v2.y - v1.y)}
    if(y == v1.y && v1.y < v2.y) {
      xs = interpolate(x);
      if constexpr(Flags & Render::Color) rs = interpolate(r), gs = interpolate(g), bs = interpolate(b);
      if constexpr(Flags & Render::Texel) us = interpolate(u), vs = interpolate(v);
      if(v1.x <= v2.x) {
        swap(xs.l, xs.r);
        if constexpr(Flags & Render::Color) swap(rs.l, rs.r), swap(gs.l, gs.r), swap(bs.l, bs.r);
        if constexpr(Flags & Render::Texel) swap(us.l, us.r), swap(vs.l, vs.r);
      }
    }
    #undef interpolate

    #define interpolate(f) (xy.r - xy.l >> 16) ? (f.r - f.l) / (xy.r - xy.l >> 16) : 0
    i32 rx, gx, bx, ux, vx;
    if constexpr(Flags & Render::Color) rx = interpolate(ry), gx = interpolate(gy), bx = interpolate(by);
    if constexpr(Flags & Render::Texel) ux = interpolate(uy), vx = interpolate(vy);
    #undef interpolate
    i32 r = ry.l, g = gy.l, b = by.l, u = uy.l, v = vy.l;

    for(i32 x = xy.l >> 16; x < xy.r >> 16; x++) {
      if constexpr(Flags == Render::Solid) {
        renderPixel({x, y}, v0);
      }
      if constexpr(Flags == Render::Color) {
        renderPixel({x, y}, {r >> 16, g >> 16, b >> 16});
        r += rx, g += gx, b += bx;
      }
      if constexpr(Flags == Render::Texel) {
        i32 tx = (u >> 16) & ~(io.textureWindowMaskX * 8) | (io.textureWindowOffsetX & io.textureWindowMaskX) * 8;
        i32 ty = (v >> 16) & ~(io.textureWindowMaskY * 8) | (io.textureWindowOffsetY & io.textureWindowMaskY) * 8;
        if(io.textureDepth == 0) {  //4bpp
          i32 ix = (tx / 4 + ts.px) & 1023;
          i32 iy = (ty + ts.py) & 511;
          u16 index = vram.readHalf((iy * 1024 + ix) * 2);
          u16 entry = index >> ((tx & 3) * 4) & 15;
          i32 cx = ts.cx + entry & 1023;
          i32 cy = ts.cy;
          if(u16 texel = vram.readHalf((cy * 1024 + cx) * 2)) {
            u8 r = (texel >>  0 & 31) << 19;
            u8 g = (texel >>  5 & 31) << 11;
            u8 b = (texel >> 10 & 31) <<  3;
            renderPixel({x, y}, {r, g, b});
          }
        }
        u += ux, v += vx;
      }
    }

    xy.l += xs.l, xy.r += xs.r;
    if constexpr(Flags & Render::Color) {
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
auto GPU::renderTriangle(Vertex v0, Vertex v1, Vertex v2, Texture ts) -> void {
  Point vmin{min(v0.x, v1.x, v2.x), min(v0.y, v1.y, v2.y)};
  Point vmax{max(v0.x, v1.x, v2.x), max(v0.y, v1.y, v2.y)};

  i32 w = weight(v0, v1, v2);  //<0 = counter-clockwise; 0 = colinear, >0 = clockwise
  if(w < 0) swap(v1, v2), w = -w;  //make clockwise

  Point d0{v1.y - v2.y, v2.x - v1.x};
  Point d1{v2.y - v0.y, v0.x - v2.x};
  Point d2{v0.y - v1.y, v1.x - v0.x};

  Point w0{0, weight(v1, v2, vmin)};
  Point w1{0, weight(v2, v0, vmin)};
//Point w2{0, weight(v0, v1, vmin)};
  Point w2{0, w - w0.y - w1.y};

  Point vp;
  for(vp.y = vmin.y; vp.y < vmax.y; vp.y++) {
    w0.x = w0.y, w1.x = w1.y, w2.x = w2.y;
    for(vp.x = vmin.x; vp.x < vmax.x; vp.x++) {
      if((w0.x | w1.x | w2.x) >= 0) {
        if(Flags == Render::Solid) {
          renderPixel(vp, v0);
        }
        if(Flags == Render::Color) {
          u8 r = (v0.r * w0.x + v1.r * w1.x + v2.r * w2.x) / float(w);
          u8 g = (v0.g * w0.x + v1.g * w1.x + v2.g * w2.x) / float(w);
          u8 b = (v0.b * w0.x + v1.b * w1.x + v2.b * w2.x) / float(w);
          renderPixel(vp, {r, g, b});
        }
        if(Flags == Render::Texel) {
          u8 u = (v0.u * w0.x + v1.u * w1.x + v2.u * w2.x) / float(w);
          u8 v = (v0.v * w0.x + v1.v * w1.x + v2.v * w2.x) / float(w);
          i32 tx = u & ~(io.textureWindowMaskX * 8) | (io.textureWindowOffsetX & io.textureWindowMaskX) * 8;
          i32 ty = v & ~(io.textureWindowMaskY * 8) | (io.textureWindowOffsetY & io.textureWindowMaskY) * 8;
          if(io.textureDepth == 0) {  //4bpp
            i32 ix = (tx / 4 + ts.px) & 1023;
            i32 iy = (ty + ts.py) & 511;
            u16 index = vram.readHalf((iy * 1024 + ix) * 2);
            u16 entry = index >> ((tx & 3) * 4) & 15;
            i32 cx = ts.cx + entry & 1023;
            i32 cy = ts.cy;
            if(u16 texel = vram.readHalf((cy * 1024 + cx) * 2)) {
              u8 r = (texel >>  0 & 31) << 19;
              u8 g = (texel >>  5 & 31) << 11;
              u8 b = (texel >> 10 & 31) <<  3;
              renderPixel(vp, {r, g, b});
            }
          }
        }
      }

      w0.x += d0.x, w1.x += d1.x, w2.x += d2.x;
    }
    w0.y += d0.y, w1.y += d1.y, w2.y += d2.y;
  }
}
#endif
