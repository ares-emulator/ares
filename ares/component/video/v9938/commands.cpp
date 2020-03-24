auto V9938::command(uint8 data) -> void {
  if(!data.bit(4,7)) return (void)(op.executing = 0);  //stop
  if(op.executing) return;  //busy

  op.lgm = 0;
  if(g4()) op.lgm = 4;
  if(g5()) op.lgm = 5;
  if(g6()) op.lgm = 6;
  if(g7()) op.lgm = 7;
  if(!op.lgm) return;  //invalid

  op.logic     = data.bit(0,3);
  op.command   = data.bit(4,7);
  op.found     = 0;
  op.executing = 1;
  relatch();
}

auto V9938::command() -> void {
  if(!op.executing) return;  //idle

  switch(op.command) {
  case 0x0: return (void)(op.executing = 0);  //stop
  case 0x1: return (void)(op.executing = 0);  //invalid
  case 0x2: return (void)(op.executing = 0);  //invalid
  case 0x3: return (void)(op.executing = 0);  //invalid
  case 0x4: return point();
  case 0x5: return pset();
  case 0x6: return srch();
  case 0x7: return line();
  case 0x8: return lmmv();
  case 0x9: return lmmm();
  case 0xa: return lmcm();
  case 0xb: return lmmc();
  case 0xc: return hmmv();
  case 0xd: return hmmm();
  case 0xe: return ymmm();
  case 0xf: return hmmc();
  }
}

auto V9938::relatch() -> void {
  if(op.lgm == 4) {
    op.lsx   = (uint8)op.sx & ~1;
    op.ldx   = (uint8)op.dx & ~1;
    op.lnx   = op.nx & ~1;
  } else if(op.lgm == 5) {
    op.lsx   = (uint9)op.sx & ~3;
    op.ldx   = (uint9)op.dx & ~3;
    op.lnx   = op.nx & ~3;
  } else if(op.lgm == 6) {
    op.lsx   = (uint9)op.sx & ~1;
    op.ldx   = (uint9)op.dx & ~1;
    op.lnx   = op.nx & ~1;
  } else if(op.lgm == 7) {
    op.lsx   = (uint8)op.sx & ~0;
    op.ldx   = (uint8)op.dx & ~0;
    op.lnx   = op.nx & ~0;
  }
}

auto V9938::advance() -> bool {
  op.lsx++;
  op.ldx++;
  if(!--op.lnx) {
    op.sy++;
    op.dy++;
    relatch();
    if(!--op.ny) {
      op.executing = 0;
    }
    return true;
  }
  return false;
}

auto V9938::logic(uint8 dc, uint8 sc) -> uint8 {
  switch(op.logic) {
  case  0: dc = sc; break;
  case  1: dc = sc & dc; break;
  case  2: dc = sc | dc; break;
  case  3: dc = sc ^ dc; break;
  case  4: dc = ~sc; break;
  case  5: break;  //invalid
  case  6: break;  //invalid
  case  7: break;  //invalid
  case  8: if(sc) dc = sc; break;
  case  9: if(sc) dc = sc & dc; break;
  case 10: if(sc) dc = sc | dc; break;
  case 11: if(sc) dc = sc ^ dc; break;
  case 12: if(sc) dc = ~sc; break;
  case 13: break;  //undefined
  case 14: break;  //undefined
  case 15: break;  //undefined
  }
  return dc;
}

auto V9938::address(uint9 x, uint10 y) -> uint17 {
  if(op.lgm == 4) return y * 256 + (uint8)x >> 1;
  if(op.lgm == 5) return y * 512 + (uint9)x >> 2;
  if(op.lgm == 6) return y * 512 + (uint9)x >> 1;
  if(op.lgm == 7) return y * 256 + (uint8)x >> 0;
  unreachable;
}

auto V9938::read(uint1 source, uint9 x, uint10 y) -> uint8 {
  auto& ram = !source ? videoRAM : expansionRAM;
  uint8 byte = ram.read(address(x, y));
  if(op.lgm == 4) return uint4(byte >> (~x & 1) * 4);
  if(op.lgm == 5) return uint2(byte >> (~x & 3) * 2);
  if(op.lgm == 6) return uint4(byte >> (~x & 1) * 4);
  if(op.lgm == 7) return uint8(byte >> (~x & 0) * 8);
  unreachable;
}

auto V9938::write(uint1 source, uint9 x, uint10 y, uint8 data) -> void {
  auto& ram = !source ? videoRAM : expansionRAM;
  uint8 byte = ram.read(address(x, y));
  uint8 lo, hi;
  if(op.lgm == 4) lo = (~x & 1) * 4, hi = lo + 3;
  if(op.lgm == 5) lo = (~x & 3) * 2, hi = lo + 1;
  if(op.lgm == 6) lo = (~x & 1) * 4, hi = lo + 3;
  if(op.lgm == 7) lo = (~x & 0) * 8, hi = lo + 7;
  byte.bit(lo, hi) = data;
  ram.write(address(x, y), byte);
}

//read pixel from VRAM to VDP
auto V9938::point() -> void {
  op.cr = read(op.mxs, op.lsx, op.sy);
  op.executing = 0;
}

//write pixel from VDP to VRAM
auto V9938::pset() -> void {
  uint8 sc = op.cr;
  uint8 dc = read(op.mxd, op.ldx, op.dy);
  dc = logic(dc, sc);
  write(op.mxd, op.ldx, op.dy, dc);
  op.executing = 0;
}

//search from VDP to VRAM
auto V9938::srch() -> void {
  uint8 sc = read(op.mxs, op.lsx, op.sy);
  if((op.eq && sc == op.cr) || (!op.eq && sc != op.cr)) {
    op.found = 1;
    op.match = op.lsx;
    op.executing = 0;
    return;
  }
  if(advance()) op.executing = 0;
}

//draw line from VDP to VRAM
//note: no timing emulation currently
//algorithm is just bresenham, which is probably not right, but it looks okay
auto V9938::line() -> void {
  int nx = op.nx;
  int ny = op.ny;
  if(op.maj) swap(nx, ny);

  int x0 = op.dx;
  int y0 = op.dy;

  int x1 = op.dx + (!op.dix ? +nx : -nx);
  int y1 = op.dy + (!op.diy ? +ny : -ny);

  int dx = abs(x1 - x0), sx = x0 < x1 ? +1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? +1 : -1;

  int error = (dx > dy ? +dx : -dy) / 2;

  while(true) {
    uint8 sc = op.cr;
    uint8 dc = read(op.mxd, x0, y0);
    dc = logic(dc, sc);
    write(op.mxd, x0, y0, dc);

    if(x0 == x1 && y0 == y1) break;
    int e = error;
    if(e > -dx) { error -= dy; x0 += sx; }
    if(e < +dy) { error += dx; y0 += sy; }
  }

  op.executing = 0;
}

//logical move VDP to VRAM
auto V9938::lmmv() -> void {
  uint8 sc = op.cr;
  uint8 dc = read(op.mxd, op.ldx, op.dy);
  dc = logic(dc, sc);
  write(op.mxd, op.ldx, op.dy, dc);
  advance();
}

//logical move VRAM to VRAM
auto V9938::lmmm() -> void {
  uint8 sc = read(op.mxs, op.lsx, op.sy);
  uint8 dc = read(op.mxd, op.ldx, op.dy);
  dc = logic(dc, sc);
  write(op.mxd, op.ldx, op.dy, dc);
  advance();
}

//logical move VRAM to CPU
auto V9938::lmcm() -> void {
  if(op.ready) return;
  op.ready = 0;
  uint8 sc = read(op.mxs, op.lsx, op.sy);
  uint8 dc = op.cr;
  dc = logic(dc, sc);
  op.cr = dc;
  advance();
}

//logical move CPU to VRAM
auto V9938::lmmc() -> void {
  if(op.ready) return;
  op.ready = 0;
  uint8 sc = op.cr;
  uint8 dc = read(op.mxd, op.ldx, op.dy);
  write(op.mxd, op.ldx, op.dy, dc);
  advance();
}

//high-speed move VDP to VRAM
auto V9938::hmmv() -> void {
  write(op.mxd, op.ldx, op.dy, op.cr);
  advance();
}

//high-speed move VRAM to VRAM
auto V9938::hmmm() -> void {
  uint8 data = read(op.mxs, op.lsx, op.sy);
  write(op.mxd, op.ldx, op.dy, data);
  advance();
}

//high-speed move VRAM to VRAM, Y-coordinate only
auto V9938::ymmm() -> void {
  uint8 data = read(op.mxd, op.ldx, op.sy);
  write(op.mxd, op.ldx, op.dy, data);
  advance();
}

//high-speed move CPU to VRAM
auto V9938::hmmc() -> void {
  if(op.ready) return;
  op.ready = 0;
  write(op.mxd, op.ldx, op.dy, op.cr);
  advance();
}
