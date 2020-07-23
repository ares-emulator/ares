auto CPU::GTE::constructTable() -> void {
  for(uint n : range(256)) {
    unsignedNewtonRaphsonTable[n] = max(0, (0x40000 / (n + 0x100) + 1) / 2 - 0x101);
  }
  unsignedNewtonRaphsonTable[256] = 0;
}

auto CPU::GTE::clearFlags() -> void {
  flag.value = 0;
}

auto CPU::GTE::updateError() -> void {
  flag.error = bool(flag.value & 0x7f87'e000);
}

//

auto CPU::GTE::getDataRegister(uint index) -> u32 {
  u32 data;
  switch(index) {
  case  0: data = u16(v.a.x) << 0 | u16(v.a.y) << 16; break;
  case  1: data = u16(v.a.z) << 0; break;
  case  2: data = u16(v.b.x) << 0 | u16(v.b.y) << 16; break;
  case  3: data = u16(v.b.z) << 0; break;
  case  4: data = u16(v.c.x) << 0 | u16(v.c.y) << 16; break;
  case  5: data = u16(v.c.z) << 0; break;
  case  6: data = rgbc.r << 0 | rgbc.g << 8 | rgbc.b << 16 | rgbc.t << 24; break;
  case  7: data = otz; break;
  case  8: data = ir.t; break;
  case  9: data = ir.x; break;
  case 10: data = ir.y; break;
  case 11: data = ir.z; break;
  case 12: data = u16(screen[0].x) << 0 | u16(screen[0].y) << 16; break;
  case 13: data = u16(screen[1].x) << 0 | u16(screen[1].y) << 16; break;
  case 14: data = u16(screen[2].x) << 0 | u16(screen[2].y) << 16; break;
  case 15: data = u16(screen[2].x) << 0 | u16(screen[2].y) << 16; break;  //not screen[3]
  case 16: data = screen[0].z; break;
  case 17: data = screen[1].z; break;
  case 18: data = screen[2].z; break;
  case 19: data = screen[3].z; break;
  case 20: data = rgb[0]; break;
  case 21: data = rgb[1]; break;
  case 22: data = rgb[2]; break;
  case 23: data = rgb[3]; break;
  case 24: data = mac.t; break;
  case 25: data = mac.x; break;
  case 26: data = mac.y; break;
  case 27: data = mac.z; break;
  case 28:  //IRGB
  case 29: {//ORGB
    u8 r = uclamp<5>(ir.x >> 7);
    u8 g = uclamp<5>(ir.y >> 7);
    u8 b = uclamp<5>(ir.z >> 7);
    data = r << 0 | g << 5 | b << 10;
  } break;
  case 30: data = lzcs; break;
  case 31: data = lzcr; break;
  }
  return data;
}

auto CPU::GTE::setDataRegister(uint index, u32 data) -> void {
  switch(index) {
  case  0: v.a.x = data >> 0; v.a.y = data >> 16; break;
  case  1: v.a.z = data >> 0; break;
  case  2: v.b.x = data >> 0; v.b.y = data >> 16; break;
  case  3: v.b.z = data >> 0; break;
  case  4: v.c.x = data >> 0; v.c.y = data >> 16; break;
  case  5: v.c.z = data >> 0; break;
  case  6: rgbc.r = data >> 0; rgbc.g = data >> 8; rgbc.b = data >> 16; rgbc.t = data >> 24; break;
  case  7: otz = data; break;
  case  8: ir.t = data; break;
  case  9: ir.x = data; break;
  case 10: ir.y = data; break;
  case 11: ir.z = data; break;
  case 12: screen[0].x = data >> 0; screen[0].y = data >> 16; break;
  case 13: screen[1].x = data >> 0; screen[1].y = data >> 16; break;
  case 14: screen[2].x = data >> 0; screen[2].y = data >> 16; break;
  case 15: {//SXP
    screen[0].x = screen[1].x; screen[0].y = screen[1].y;
    screen[1].x = screen[2].x; screen[1].y = screen[2].y;
    screen[2].x = data >> 0;   screen[2].y = data >> 16;
  } break;
  case 16: screen[0].z = data; break;
  case 17: screen[1].z = data; break;
  case 18: screen[2].z = data; break;
  case 19: screen[3].z = data; break;
  case 20: rgb[0] = data; break;
  case 21: rgb[1] = data; break;
  case 22: rgb[2] = data; break;
  case 23: rgb[3] = data; break;
  case 24: mac.t = data; break;
  case 25: mac.x = data; break;
  case 26: mac.y = data; break;
  case 27: mac.z = data; break;
  case 28: irgb = data; break;
  case 29: orgb = data; break;
  case 30: lzcs = data; break;
  case 31: lzcr = data; break;
  }
}

auto CPU::GTE::getControlRegister(uint index) -> u32 {
  u32 data;
  switch(index) {
  case  0: data = u16(rotation.a.x) << 0 | u16(rotation.a.y) << 16; break;
  case  1: data = u16(rotation.a.z) << 0 | u16(rotation.b.x) << 16; break;
  case  2: data = u16(rotation.b.y) << 0 | u16(rotation.b.z) << 16; break;
  case  3: data = u16(rotation.c.x) << 0 | u16(rotation.c.y) << 16; break;
  case  4: data = u16(rotation.c.z) << 0; break;
  case  5: data = translation.x; break;
  case  6: data = translation.y; break;
  case  7: data = translation.z; break;
  case  8: data = u16(light.a.x) << 0 | u16(light.a.y) << 16; break;
  case  9: data = u16(light.a.z) << 0 | u16(light.b.x) << 16; break;
  case 10: data = u16(light.b.y) << 0 | u16(light.b.z) << 16; break;
  case 11: data = u16(light.c.x) << 0 | u16(light.c.y) << 16; break;
  case 12: data = u16(light.c.z) << 0; break;
  case 13: data = backgroundColor.r; break;
  case 14: data = backgroundColor.g; break;
  case 15: data = backgroundColor.b; break;
  case 16: data = u16(color.a.x) << 0 | u16(color.a.y) << 16; break;
  case 17: data = u16(color.a.z) << 0 | u16(color.b.x) << 16; break;
  case 18: data = u16(color.b.y) << 0 | u16(color.b.z) << 16; break;
  case 19: data = u16(color.c.x) << 0 | u16(color.c.y) << 16; break;
  case 20: data = u16(color.c.z) << 0; break;
  case 21: data = farColor.r; break;
  case 22: data = farColor.g; break;
  case 23: data = farColor.b; break;
  case 24: data = ofx; break;
  case 25: data = ofy; break;
  case 26: data = h; break;
  case 27: data = dqa; break;
  case 28: data = dqb; break;
  case 29: data = zsf3; break;
  case 30: data = zsf4; break;
  case 31: data = flag.value; break;
  }
  return data;
}

auto CPU::GTE::setControlRegister(uint index, u32 data) -> void {
  switch(index) {
  case  0: rotation.a.x = data >> 0; rotation.a.y = data >> 16; break;
  case  1: rotation.a.z = data >> 0; rotation.b.x = data >> 16; break;
  case  2: rotation.b.y = data >> 0; rotation.b.z = data >> 16; break;
  case  3: rotation.c.x = data >> 0; rotation.c.y = data >> 16; break;
  case  4: rotation.c.z = data >> 0; break;
  case  5: translation.x = data; break;
  case  6: translation.y = data; break;
  case  7: translation.z = data; break;
  case  8: light.a.x = data >> 0; light.a.y = data >> 16; break;
  case  9: light.a.z = data >> 0; light.b.x = data >> 16; break;
  case 10: light.b.y = data >> 0; light.b.z = data >> 16; break;
  case 11: light.c.x = data >> 0; light.c.y = data >> 16; break;
  case 12: light.c.z = data >> 0; break;
  case 13: backgroundColor.r = data; break;
  case 14: backgroundColor.g = data; break;
  case 15: backgroundColor.b = data; break;
  case 16: color.a.x = data >> 0; color.a.y = data >> 16; break;
  case 17: color.a.z = data >> 0; color.b.x = data >> 16; break;
  case 18: color.b.y = data >> 0; color.b.z = data >> 16; break;
  case 19: color.c.x = data >> 0; color.c.y = data >> 16; break;
  case 20: color.c.z = data >> 0; break;
  case 21: farColor.r = data; break;
  case 22: farColor.g = data; break;
  case 23: farColor.b = data; break;
  case 24: ofx = data; break;
  case 25: ofy = data; break;
  case 26: h = data; break;
  case 27: dqa = data; break;
  case 28: dqb = data; break;
  case 29: zsf3 = data; break;
  case 30: zsf4 = data; break;
  case 31: flag.value = data & 0xffff'f000; break;
  }
}

//

template<uint id>
auto CPU::GTE::check(i64 value) -> i64 {
  static constexpr i64 min = -(i64(1) << (id == 0 ? 31 : 43));
  static constexpr i64 max = +(i64(1) << (id == 0 ? 31 : 43)) - 1;
  if(value < min) {
    if constexpr(id == 0) flag.mac0_underflow = 1;
    if constexpr(id == 1) flag.mac1_underflow = 1;
    if constexpr(id == 2) flag.mac2_underflow = 1;
    if constexpr(id == 3) flag.mac3_underflow = 1;
  }
  if(value > max) {
    if constexpr(id == 0) flag.mac0_overflow = 1;
    if constexpr(id == 1) flag.mac1_overflow = 1;
    if constexpr(id == 2) flag.mac2_overflow = 1;
    if constexpr(id == 3) flag.mac3_overflow = 1;
  }
  return value;
}

template<uint id>
auto CPU::GTE::extend(i64 mac) -> i64 {
  check<id>(mac);
  if constexpr(id == 0) return sclip<31>(mac);
  if constexpr(id == 1) return sclip<44>(mac);
  if constexpr(id == 2) return sclip<44>(mac);
  if constexpr(id == 3) return sclip<44>(mac);
  unreachable;
}

template<uint id>
auto CPU::GTE::saturate(i32 value, bool lm) -> i32 {
  static constexpr i32 min = id == 0 ? 0x0000 : -0x8000;
  static constexpr i32 max = id == 0 ? 0x1000 : +0x7fff;
  i32 lmm = lm ? 0 : min;
  if(value < lmm) {
    value = lmm;
    if constexpr(id == 0) flag.ir0_saturated = 1;
    if constexpr(id == 1) flag.ir1_saturated = 1;
    if constexpr(id == 2) flag.ir2_saturated = 1;
    if constexpr(id == 3) flag.ir3_saturated = 1;
  }
  if(value > max) {
    value = max;
    if constexpr(id == 0) flag.ir0_saturated = 1;
    if constexpr(id == 1) flag.ir1_saturated = 1;
    if constexpr(id == 2) flag.ir2_saturated = 1;
    if constexpr(id == 3) flag.ir3_saturated = 1;
  }
  return value;
}

template<uint id>
auto CPU::GTE::saturateRGB(i32 value) -> u32 {
  if(value < 0 || value > 255) {
    if constexpr(id == 0) flag.r_saturated = 1;
    if constexpr(id == 1) flag.g_saturated = 1;
    if constexpr(id == 2) flag.b_saturated = 1;
    return uclamp<8>(value);
  }
  return value;
}

//

template<uint id>
auto CPU::GTE::setMac(i64 value) -> i64 {
  check<id>(value);
  if constexpr(id == 0) { mac.t = value;       return value;       }
  if constexpr(id == 1) { mac.x = value >> sf; return value >> sf; }
  if constexpr(id == 2) { mac.y = value >> sf; return value >> sf; }
  if constexpr(id == 3) { mac.z = value >> sf; return value >> sf; }
}

template<uint id>
auto CPU::GTE::setIr(i32 value, bool lm) -> void {
  if constexpr(id == 0) ir.t = saturate<0>(value, lm);
  if constexpr(id == 1) ir.x = saturate<1>(value, lm);
  if constexpr(id == 2) ir.y = saturate<2>(value, lm);
  if constexpr(id == 3) ir.z = saturate<3>(value, lm);
}

template<uint id>
auto CPU::GTE::setMacAndIr(i64 value, bool lm) -> void {
  setIr<id>(setMac<id>(value), lm);
}

auto CPU::GTE::setMacAndIr(const v64& vector) -> void {
  setMacAndIr<1>(vector.x, lm);
  setMacAndIr<2>(vector.y, lm);
  setMacAndIr<3>(vector.z, lm);
}

auto CPU::GTE::setOtz(i64 value) -> void {
  static constexpr i64 min = 0x0000;
  static constexpr i64 max = 0xffff;
  value >>= 12;
  if(value < min) { otz = min; flag.otz_saturated = 1; return; }
  if(value > max) { otz = max; flag.otz_saturated = 1; return; }
  otz = value;
}

//

auto CPU::GTE::matrixMultiply(const m16& matrix, const v16& vector, const v32& translation) -> v64 {
  i64 x = extend<1>(extend<1>(extend<1>((i64(translation.x) << 12) + matrix.a.x * vector.x) + matrix.a.y * vector.y) + matrix.a.z * vector.z);
  i64 y = extend<1>(extend<1>(extend<1>((i64(translation.y) << 12) + matrix.b.x * vector.x) + matrix.b.y * vector.y) + matrix.b.z * vector.z);
  i64 z = extend<1>(extend<1>(extend<1>((i64(translation.z) << 12) + matrix.c.x * vector.x) + matrix.c.y * vector.y) + matrix.c.z * vector.z);
  return {x, y, z};
}

auto CPU::GTE::vectorMultiply(const v16& vector1, const v16& vector2, const v16& translation) -> v64 {
  i64 x = (i64(translation.x) << 12) + vector1.x * vector2.x;
  i64 y = (i64(translation.y) << 12) + vector1.y * vector2.y;
  i64 z = (i64(translation.z) << 12) + vector1.z * vector2.z;
  return {x, y, z};
}

auto CPU::GTE::divide(u32 lhs, u32 rhs) -> u32 {
  if(rhs * 2 <= lhs) {
    flag.divide_overflow = 1;
    return 0x1ffff;
  }

  u32 shift = __builtin_clz(u16(rhs)) - 16;
  lhs <<= shift;
  rhs <<= shift;

  u32 divisor = rhs | 0x8000;
  i32 x = i32(0x101 + unsignedNewtonRaphsonTable[(divisor & 0x7fff) + 0x40 >> 7]);
  i32 d = i32(divisor) * -x + 0x80 >> 8;
  u32 reciprocal = x * (0x20000 + d) + 0x80 >> 8;
  u32 result = u64(lhs) * reciprocal + 0x8000 >> 16;
  return min(0x1ffff, result);
}

auto CPU::GTE::pushScreenX(i32 sx) -> void {
  if(sx < -1024) sx = -1024, flag.sx2_saturated = 1;
  if(sx > +1023) sx = +1023, flag.sx2_saturated = 1;
  screen[0].x = screen[1].x;
  screen[1].x = screen[2].x;
  screen[2].x = sx;
}

auto CPU::GTE::pushScreenY(i32 sy) -> void {
  if(sy < -1024) sy = -1024, flag.sy2_saturated = 1;
  if(sy > +1023) sy = +1023, flag.sy2_saturated = 1;
  screen[0].y = screen[1].y;
  screen[1].y = screen[2].y;
  screen[2].y = sy;
}

auto CPU::GTE::pushScreenZ(i32 sz) -> void {
  if(sz < 0x0000) sz = 0x0000, flag.sz3_saturated = 1;
  if(sz > 0xffff) sz = 0xffff, flag.sz3_saturated = 1;
  screen[0].z = screen[1].z;
  screen[1].z = screen[2].z;
  screen[2].z = screen[3].z;
  screen[3].z = sz;
}

auto CPU::GTE::pushColor(u32 r, u32 g, u32 b) -> void {
  r = saturateRGB<1>(r);
  g = saturateRGB<2>(g);
  b = saturateRGB<3>(b);

  rgb[0] = rgb[1];
  rgb[1] = rgb[2];
  rgb[2] = r << 0 | g << 8 | b << 16 | rgbc.t << 24;
}

auto CPU::GTE::pushColor() -> void {
  pushColor(mac.r >> 4, mac.g >> 4, mac.b >> 4);
}

//

auto CPU::GTE::avsz3() -> void {
  setOtz(setMac<0>(i64(zsf3) * (screen[1].z + screen[2].z + screen[3].z)));
}

auto CPU::GTE::avsz4() -> void {
  setOtz(setMac<0>(i64(zsf4) * (screen[0].z + screen[1].z + screen[2].z + screen[3].z)));
}

auto CPU::GTE::cc() -> void {
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));
  setMacAndIr(vectorMultiply({rgbc.r << 4, rgbc.g << 4, rgbc.b << 4}, ir));
  pushColor();
}

auto CPU::GTE::cdp() -> void {
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));

  v16 i = ir;
  setMacAndIr<1>((i64(farColor.r) << 12) - ((rgbc.r << 4) * ir.x));
  setMacAndIr<2>((i64(farColor.g) << 12) - ((rgbc.g << 4) * ir.y));
  setMacAndIr<3>((i64(farColor.b) << 12) - ((rgbc.b << 4) * ir.z));

  setMacAndIr<1>(((rgbc.r << 4) * i.x) + ir.t * ir.x, lm);
  setMacAndIr<2>(((rgbc.g << 4) * i.y) + ir.t * ir.y, lm);
  setMacAndIr<3>(((rgbc.b << 4) * i.z) + ir.t * ir.z, lm);
  pushColor();
}

auto CPU::GTE::dpc(const v16& color) -> void {
  setMacAndIr<1>((i64(farColor.r) << 12) - (color.r << 12));
  setMacAndIr<2>((i64(farColor.g) << 12) - (color.g << 12));
  setMacAndIr<3>((i64(farColor.b) << 12) - (color.b << 12));

  setMacAndIr(vectorMultiply({ir.t, ir.t, ir.t}, ir, color));
  pushColor();
}

auto CPU::GTE::dcpl() -> void {
  v16 i = ir;
  v16 color = {rgbc.r << 4, rgbc.g << 4, rgbc.b << 4};
  setMacAndIr<1>((i64(farColor.r) << 12) - color.r * i.x);
  setMacAndIr<2>((i64(farColor.g) << 12) - color.g * i.y);
  setMacAndIr<3>((i64(farColor.b) << 12) - color.b * i.z);

  setMacAndIr<1>(color.r * i.x + ir.t * ir.x, lm);
  setMacAndIr<2>(color.g * i.y + ir.t * ir.y, lm);
  setMacAndIr<3>(color.b * i.z + ir.t * ir.z, lm);
  pushColor();
}

auto CPU::GTE::dpcs() -> void {
  dpc({rgbc.r << 4, rgbc.g << 4, rgbc.b << 4});
}

auto CPU::GTE::dpct() -> void {
  dpc({rgb[0] << 4, rgb[1] << 4, rgb[2] << 4});
  dpc({rgb[0] << 4, rgb[1] << 4, rgb[2] << 4});
  dpc({rgb[0] << 4, rgb[1] << 4, rgb[2] << 4});
}

auto CPU::GTE::gpf() -> void {
  setMacAndIr(vectorMultiply(ir, {ir.t, ir.t, ir.t}));
  pushColor();
}

auto CPU::GTE::gpl() -> void {
  setMacAndIr<1>((i64(mac.x) << sf) + ir.t * ir.x, lm);
  setMacAndIr<2>((i64(mac.y) << sf) + ir.t * ir.y, lm);
  setMacAndIr<3>((i64(mac.z) << sf) + ir.t * ir.z, lm);
  pushColor();
}

auto CPU::GTE::intpl() -> void {
  v16 i = ir;
  setMacAndIr<1>((i64(farColor.r) << 12) - (i64(i.x) << 12));
  setMacAndIr<2>((i64(farColor.g) << 12) - (i64(i.y) << 12));
  setMacAndIr<3>((i64(farColor.b) << 12) - (i64(i.z) << 12));

  setMacAndIr<1>((i64(ir.t) << 12) + ir.x * i.x, lm);
  setMacAndIr<2>((i64(ir.t) << 12) + ir.y * i.y, lm);
  setMacAndIr<3>((i64(ir.t) << 12) + ir.z * i.z, lm);

  pushColor();
}

auto CPU::GTE::mvmva() -> void {
  v32 tr;
  switch(tv) {
  case 0: tr = translation; break;
  case 1: tr = backgroundColor; break;
  case 2: tr = farColor; break;
  case 3: tr = {0, 0, 0}; break;
  }

  v16 vector;
  switch(mv) {
  case 0: vector = v.a; break;
  case 1: vector = v.b; break;
  case 2: vector = v.c; break;
  case 3: vector = ir; break;
  }

  m16 matrix;
  switch(mm) {
  case 0: matrix = rotation; break;
  case 1: matrix = light; break;
  case 2: matrix = color; break;
  case 3:  //reserved
    matrix.a.x = -(rgbc.r << 4); matrix.a.y = +(rgbc.r << 4); matrix.a.z = ir.t;
    matrix.b.x = rotation.a.z; matrix.b.y = rotation.a.z; matrix.b.z = rotation.a.z;
    matrix.c.x = rotation.b.y; matrix.c.y = rotation.b.y; matrix.c.z = rotation.b.y;
    break;
  }

  if(tv != 2) {
    setMacAndIr(matrixMultiply(matrix, vector, tr));
  } else {
    setIr<1>(extend<1>((i64(tr.x) << 12) + matrix.a.x * vector.x) >> sf);
    setIr<2>(extend<2>((i64(tr.y) << 12) + matrix.b.x * vector.x) >> sf);
    setIr<3>(extend<3>((i64(tr.z) << 12) + matrix.c.x * vector.x) >> sf);

    v64 result;
    result.x = extend<1>(extend<1>(matrix.a.y * vector.y) + matrix.a.z * vector.z);
    result.y = extend<1>(extend<1>(matrix.b.y * vector.y) + matrix.b.z * vector.z);
    result.z = extend<1>(extend<1>(matrix.c.y * vector.y) + matrix.c.z * vector.z);
    setMacAndIr(result);
  }
}

template<uint m>
auto CPU::GTE::nc(const v16& vector) -> void {
  setMacAndIr(matrixMultiply(light, vector));
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));

  if constexpr(m == 1) {
    setMacAndIr<1>((rgbc.r << 4) * ir.x);
    setMacAndIr<2>((rgbc.g << 4) * ir.y);
    setMacAndIr<3>((rgbc.b << 4) * ir.z);
  }

  if constexpr(m == 2) {
    v16 i = ir;
    setMacAndIr<1>((i64(farColor.r) << 12) - (rgbc.r << 4) * i.x);
    setMacAndIr<2>((i64(farColor.g) << 12) - (rgbc.g << 4) * i.y);
    setMacAndIr<3>((i64(farColor.b) << 12) - (rgbc.b << 4) * i.z);

    setMacAndIr<1>((rgbc.r << 4) * i.x + ir.t * ir.x, lm);
    setMacAndIr<2>((rgbc.g << 4) * i.y + ir.t * ir.y, lm);
    setMacAndIr<3>((rgbc.b << 4) * i.z + ir.t * ir.z, lm);
  }

  pushColor();
}

auto CPU::GTE::nccs() -> void {
  nc<1>(v.a);
}

auto CPU::GTE::ncct() -> void {
  nc<1>(v.a);
  nc<1>(v.b);
  nc<1>(v.c);
}

auto CPU::GTE::ncds() -> void {
  nc<2>(v.a);
}

auto CPU::GTE::ncdt() -> void {
  nc<2>(v.a);
  nc<2>(v.b);
  nc<2>(v.c);
}

auto CPU::GTE::nclip() -> void {
  i64 p0 = i64(screen[0].x) * i64(screen[1].y);
  i64 p1 = i64(screen[1].x) * i64(screen[2].y);
  i64 p2 = i64(screen[2].x) * i64(screen[0].y);
  i64 p3 = i64(screen[0].x) * i64(screen[2].y);
  i64 p4 = i64(screen[1].x) * i64(screen[0].y);
  i64 p5 = i64(screen[2].x) * i64(screen[1].y);
  setMac<0>(p0 + p1 + p2 - p3 - p4 - p5);
}

auto CPU::GTE::ncs() -> void {
  nc<0>(v.a);
}

auto CPU::GTE::nct() -> void {
  nc<0>(v.a);
  nc<0>(v.b);
  nc<0>(v.c);
}

auto CPU::GTE::op() -> void {
  setMac<1>(rotation.b.y * ir.z - rotation.c.z * ir.y);
  setMac<2>(rotation.c.z * ir.x - rotation.a.x * ir.z);
  setMac<3>(rotation.a.x * ir.y - rotation.b.y * ir.x);

  setIr<1>(mac.x, lm);
  setIr<2>(mac.y, lm);
  setIr<3>(mac.z, lm);
}

//rotation, translation, and perspective transformation
auto CPU::GTE::rtp(v16 vector, bool last) -> void {
  v64 result = matrixMultiply(rotation, vector, translation);
  setMacAndIr<1>(result.x, lm);
  setMacAndIr<2>(result.y, lm);
  setMac<3>(result.z);
  saturate<3>(result.z >> 12);
  ir.z = std::clamp(mac.z, lm ? 0 : -0x8000, +0x7fff);

  pushScreenZ(result.z >> 12);
  i64 dv = divide(h, screen[3].z);
  i32 sx = setMac<0>(dv * ir.x + ofx);
  i32 sy = setMac<0>(dv * ir.y + ofy);
  pushScreenX(sx >> 16);
  pushScreenY(sy >> 16);
  if(!last) return;

  i64 sz = setMac<0>(dv * dqa + dqb);
  ir.t = saturate<0>(sz >> 12);
}

auto CPU::GTE::rtps() -> void {
  rtp(v.a, 1);
}

auto CPU::GTE::rtpt() -> void {
  rtp(v.a, 0);
  rtp(v.b, 0);
  rtp(v.c, 1);
}

auto CPU::GTE::sqr() -> void {
  setMacAndIr(vectorMultiply(ir, ir));
}
