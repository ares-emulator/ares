auto GPU::renderPixel(Point p, Color c) -> void {
  u32 address = (p.y & 511) * 1024 + (p.x & 1023);
  u16 color = (c.r >> 3) << 10 | (c.g >> 3) << 5 | (c.b >> 3) << 0;
  vram.writeHalf(address * 2, color);
}

auto GPU::renderSolidLine(Point p0, Point p1, Color c) -> void {
  Point d = {p1.x - p0.x, p1.y - p0.y};
  u32 steps = abs(d.x) > abs(d.y) ? abs(d.x) : abs(d.y);
  Point s = {(d.x << 16) / steps, (d.y << 16) / steps};
  Point p = {p0.x << 16, p0.y << 16};

  for(u16 step : range(steps)) {
    renderPixel({p.x >> 16, p.y >> 16}, c);
    p.x += s.x;
    p.y += s.y;
  }
}

auto GPU::renderSolidOpaqueTriangle(Point p0, Point p1, Point p2, Color c) -> void {
  //x0 <= x1 <= x2
  Point x0 = p0, x1 = p1, x2 = p2;
  if(x1.x < x0.x) swap(x1, x0);
  if(x2.x < x0.x) swap(x2, x0);
  if(x2.x < x1.x) swap(x2, x1);

  //y0 <= y1 <= y2
  Point y0 = p0, y1 = p1, y2 = p2;
  if(y1.y < y0.y) swap(y1, y0);
  if(y2.y < y0.y) swap(y2, y0);
  if(y2.y < y1.y) swap(y2, y1);

  if(y1.y > y0.y) {
    i32 sl = (x1.x - x0.x << 16) / (y1.y - y0.y);
    i32 sr = (x2.x - x1.x << 16) / (y1.y - y0.y);
    i32 xl = y0.x << 16;
    i32 xr = y0.x << 16;
    for(i32 y = y0.y; y <= y1.y; y++) {
      for(i32 x = xl >> 16; x <= xr >> 16; x++) {
        renderPixel({x, y}, c);
      }
      xl -= sl;
      xr += sr;
    }
  }

  if(y2.y > y1.y) {
    i32 sl = (x1.x - x0.x << 16) / (y2.y - y1.y);
    i32 sr = (x2.x - x1.x << 16) / (y2.y - y1.y);
    i32 xl = y2.x << 16;
    i32 xr = y2.x << 16;
    for(i32 y = y2.y; y > y1.y; y--) {
      for(i32 x = xl >> 16; x <= xr >> 16; x++) {
        renderPixel({x, y}, c);
      }
      xl -= sl;
      xr += sr;
    }
  }
}

auto GPU::renderShadedOpaqueTriangle(Vertex v0, Vertex v1, Vertex v2) -> void {
  //x0 <= x1 <= x2
  Vertex x0 = v0, x1 = v1, x2 = v2;
  if(x1.x < x0.x) swap(x1, x0);
  if(x2.x < x0.x) swap(x2, x0);
  if(x2.x < x1.x) swap(x2, x1);

  //y0 <= y1 <= y2
  Vertex y0 = v0, y1 = v1, y2 = v2;
  if(y1.y < y0.y) swap(y1, y0);
  if(y2.y < y0.y) swap(y2, y0);
  if(y2.y < y1.y) swap(y2, y1);

  if(y1.y > y0.y) {
    i32 sl = (x1.x - x0.x << 16) / (y1.y - y0.y);
    i32 sr = (x2.x - x1.x << 16) / (y1.y - y0.y);
    i32 xl = y0.x << 16;
    i32 xr = y0.x << 16;
    for(i32 y = y0.y; y <= y1.y; y++) {
      for(i32 x = xl >> 16; x <= xr >> 16; x++) {
        renderPixel({x, y}, {v0.r, v0.g, v0.b});
      }
      xl -= sl;
      xr += sr;
    }
  }

  if(y2.y > y1.y) {
    i32 sl = (x1.x - x0.x << 16) / (y2.y - y1.y);
    i32 sr = (x2.x - x1.x << 16) / (y2.y - y1.y);
    i32 xl = y2.x << 16;
    i32 xr = y2.x << 16;
    for(i32 y = y2.y; y > y1.y; y--) {
      for(i32 x = xl >> 16; x <= xr >> 16; x++) {
        renderPixel({x, y}, {v0.r, v0.g, v0.b});
      }
      xl -= sl;
      xr += sr;
    }
  }
}

auto GPU::renderSolidOpaqueQuadrilateral(Point p0, Point p1, Point p2, Point p3, Color c) -> void {
  renderSolidOpaqueTriangle(p0, p1, p2, c);
  renderSolidOpaqueTriangle(p1, p2, p3, c);
}

auto GPU::renderShadedOpaqueQuadrilateral(Vertex v0, Vertex v1, Vertex v2, Vertex v3) -> void {
  renderShadedOpaqueTriangle(v0, v1, v2);
  renderShadedOpaqueTriangle(v1, v2, v3);
}
