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
  case  0: data = u16(vx0) << 0 | u16(vy0) << 16; break;
  case  1: data = u16(vz0) << 0; break;
  case  2: data = u16(vx1) << 0 | u16(vy1) << 16; break;
  case  3: data = u16(vz1) << 0; break;
  case  4: data = u16(vx2) << 0 | u16(vy2) << 16; break;
  case  5: data = u16(vz2) << 0; break;
  case  6: data = rgbc; break;
  case  7: data = otz; break;
  case  8: data = ir0; break;
  case  9: data = ir1; break;
  case 10: data = ir2; break;
  case 11: data = ir3; break;
  case 12: data = u16(sx0) << 0 | u16(sy0) << 16; break;
  case 13: data = u16(sx1) << 0 | u16(sy1) << 16; break;
  case 14: data = u16(sx2) << 0 | u16(sy2) << 16; break;
  case 15: data = u16(sx2) << 0 | u16(sy2) << 16; break;
  case 16: data = sz0; break;
  case 17: data = sz1; break;
  case 18: data = sz2; break;
  case 19: data = sz3; break;
  case 20: data = rgb0; break;
  case 21: data = rgb1; break;
  case 22: data = rgb2; break;
  case 23: data = res1; break;
  case 24: data = mac0; break;
  case 25: data = mac1; break;
  case 26: data = mac2; break;
  case 27: data = mac3; break;
  case 28:  //IRGB
  case 29: {//ORGB
    u8 r = uclamp<5>(ir1 >> 7);
    u8 g = uclamp<5>(ir2 >> 7);
    u8 b = uclamp<5>(ir3 >> 7);
    data = r << 0 | g << 5 | b << 10;
  } break;
  case 30: data = lzcs; break;
  case 31: data = lzcr; break;
  }
  return data;
}

auto CPU::GTE::setDataRegister(uint index, u32 data) -> void {
  switch(index) {
  case  0: vx0 = data >> 0; vy0 = data >> 16; break;
  case  1: vz0 = data >> 0; break;
  case  2: vx1 = data >> 0; vy1 = data >> 16; break;
  case  3: vz1 = data >> 0; break;
  case  4: vx2 = data >> 0; vy2 = data >> 16; break;
  case  5: vz2 = data >> 0; break;
  case  6: rgbc = data; break;
  case  7: otz = data; break;
  case  8: ir0 = data; break;
  case  9: ir1 = data; break;
  case 10: ir2 = data; break;
  case 11: ir3 = data; break;
  case 12: sx0 = data >> 0; sy0 = data >> 16; break;
  case 13: sx1 = data >> 0; sy1 = data >> 16; break;
  case 14: sx2 = data >> 0; sy2 = data >> 16; break;
  case 15: {//SXP
    sx0 = sx1; sy0 = sy1;
    sx1 = sx2; sy1 = sy2;
    sx2 = data >> 0; sy2 = data >> 16;
  } break;
  case 16: sz0 = data; break;
  case 17: sz1 = data; break;
  case 18: sz2 = data; break;
  case 19: sz3 = data; break;
  case 20: rgb0 = data; break;
  case 21: rgb1 = data; break;
  case 22: rgb2 = data; break;
  case 23: res1 = data; break;
  case 24: mac0 = data; break;
  case 25: mac1 = data; break;
  case 26: mac2 = data; break;
  case 27: mac3 = data; break;
  case 28: irgb = data; break;
  case 29: orgb = data; break;
  case 30: lzcs = data; break;
  case 31: lzcr = data; break;
  }
}

auto CPU::GTE::getControlRegister(uint index) -> u32 {
  u32 data;
  switch(index) {
  case  0: data = u16(rt11) << 0 | u16(rt12) << 16; break;
  case  1: data = u16(rt13) << 0 | u16(rt21) << 16; break;
  case  2: data = u16(rt22) << 0 | u16(rt23) << 16; break;
  case  3: data = u16(rt31) << 0 | u16(rt32) << 16; break;
  case  4: data = u16(rt33) << 0; break;
  case  5: data = trx; break;
  case  6: data = trY; break;
  case  7: data = trz; break;
  case  8: data = u16(l11) << 0 | u16(l12) << 16; break;
  case  9: data = u16(l13) << 0 | u16(l21) << 16; break;
  case 10: data = u16(l22) << 0 | u16(l23) << 16; break;
  case 11: data = u16(l31) << 0 | u16(l32) << 16; break;
  case 12: data = u16(l33) << 0; break;
  case 13: data = rbk; break;
  case 14: data = gbk; break;
  case 15: data = bbk; break;
  case 16: data = u16(lr1) << 0 | u16(lr2) << 16; break;
  case 17: data = u16(lr3) << 0 | u16(lg1) << 16; break;
  case 18: data = u16(lg2) << 0 | u16(lg3) << 16; break;
  case 19: data = u16(lb1) << 0 | u16(lb2) << 16; break;
  case 20: data = u16(lb3) << 0; break;
  case 21: data = rfc; break;
  case 22: data = gfc; break;
  case 23: data = bfc; break;
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
  case  0: rt11 = data >> 0; rt12 = data >> 16; break;
  case  1: rt13 = data >> 0; rt21 = data >> 16; break;
  case  2: rt22 = data >> 0; rt23 = data >> 16; break;
  case  3: rt31 = data >> 0; rt32 = data >> 16; break;
  case  4: rt33 = data >> 0; break;
  case  5: trx = data; break;
  case  6: trY = data; break;
  case  7: trz = data; break;
  case  8: l11 = data >> 0; l12 = data >> 16; break;
  case  9: l13 = data >> 0; l21 = data >> 16; break;
  case 10: l22 = data >> 0; l23 = data >> 16; break;
  case 11: l31 = data >> 0; l32 = data >> 16; break;
  case 12: l33 = data >> 0; break;
  case 13: rbk = data; break;
  case 14: gbk = data; break;
  case 15: bbk = data; break;
  case 16: lr1 = data >> 0; lr2 = data >> 16; break;
  case 17: lr3 = data >> 0; lg1 = data >> 16; break;
  case 18: lg2 = data >> 0; lg3 = data >> 16; break;
  case 19: lb1 = data >> 0; lb2 = data >> 16; break;
  case 20: lb3 = data >> 0; break;
  case 21: rfc = data; break;
  case 22: gfc = data; break;
  case 23: bfc = data; break;
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

#define R (u8(rgbc >>  0) << 4)
#define G (u8(rgbc >>  8) << 4)
#define B (u8(rgbc >> 16) << 4)

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
  if constexpr(id == 0) { mac0 = value;       return value;       }
  if constexpr(id == 1) { mac1 = value >> sf; return value >> sf; }
  if constexpr(id == 2) { mac2 = value >> sf; return value >> sf; }
  if constexpr(id == 3) { mac3 = value >> sf; return value >> sf; }
}

template<uint id>
auto CPU::GTE::setIr(i32 value, bool lm) -> void {
  if constexpr(id == 0) ir0 = saturate<0>(value, lm);
  if constexpr(id == 1) ir1 = saturate<1>(value, lm);
  if constexpr(id == 2) ir2 = saturate<2>(value, lm);
  if constexpr(id == 3) ir3 = saturate<3>(value, lm);
}

template<uint id>
auto CPU::GTE::setMacAndIr(i64 value, bool lm) -> void {
  setIr<id>(setMac<id>(value), lm);
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

auto CPU::GTE::pushSX(i32 sx) -> void {
  if(sx < -1024) sx = -1024, flag.sx2_saturated = 1;
  if(sx > +1023) sx = +1023, flag.sx2_saturated = 1;
  sx0 = sx1;
  sx1 = sx2;
  sx2 = sx;
}

auto CPU::GTE::pushSY(i32 sy) -> void {
  if(sy < -1024) sy = -1024, flag.sy2_saturated = 1;
  if(sy > +1023) sy = +1023, flag.sy2_saturated = 1;
  sy0 = sy1;
  sy1 = sy2;
  sy2 = sy;
}

auto CPU::GTE::pushSZ(i32 sz) -> void {
  if(sz < 0x0000) sz = 0x0000, flag.sz3_saturated = 1;
  if(sz > 0xffff) sz = 0xffff, flag.sz3_saturated = 1;
  sz0 = sz1;
  sz1 = sz2;
  sz2 = sz3;
  sz3 = sz;
}

auto CPU::GTE::pushColor(u32 r, u32 g, u32 b) -> void {
  r = saturateRGB<1>(r);
  g = saturateRGB<2>(g);
  b = saturateRGB<3>(b);

  rgb0 = rgb1;
  rgb1 = rgb2;
  rgb2 = r << 0 | g << 8 | b << 16 | u8(rgbc >> 24) << 24;
}

auto CPU::GTE::pushColor() -> void {
  pushColor(mac1 >> 4, mac2 >> 4, mac3 >> 4);
}

//

auto CPU::GTE::avsz3() -> void {
  setOtz(setMac<0>(i64(zsf3) * (sz1 + sz2 + sz3)));
}

auto CPU::GTE::avsz4() -> void {
  setOtz(setMac<0>(i64(zsf4) * (sz0 + sz1 + sz2 + sz3)));
}

auto CPU::GTE::cc() -> void {
  print("CC\n");
}

auto CPU::GTE::cdp() -> void {
  print("CDP\n");
}

auto CPU::GTE::dpc(i16 r, i16 g, i16 b) -> void {
  setMacAndIr<1>((i64(rfc) << 12) - (r << 12));
  setMacAndIr<2>((i64(gfc) << 12) - (g << 12));
  setMacAndIr<3>((i64(bfc) << 12) - (b << 12));

  setMacAndIr<1>((i64(ir0) << 12) + ir1 * r);
  setMacAndIr<2>((i64(ir0) << 12) + ir2 * g);
  setMacAndIr<3>((i64(ir0) << 12) + ir3 * b);

  pushColor();
}

auto CPU::GTE::dcpl() -> void {
  i16 i1 = ir1;
  i16 i2 = ir2;
  i16 i3 = ir3;

  setMacAndIr<1>((i64(rfc) << 12) - R * i1);
  setMacAndIr<2>((i64(gfc) << 12) - G * i2);
  setMacAndIr<3>((i64(bfc) << 12) - B * i3);

  setMacAndIr<1>(R * i1 + ir0 * ir1, lm);
  setMacAndIr<2>(G * i2 + ir0 * ir2, lm);
  setMacAndIr<3>(B * i3 + ir0 * ir3, lm);

  pushColor();
}

auto CPU::GTE::dpcs() -> void {
  dpc(R, G, B);
}

auto CPU::GTE::dpct() -> void {
  dpc(rgb0 << 4, rgb1 << 4, rgb2 << 4);
  dpc(rgb0 << 4, rgb1 << 4, rgb2 << 4);
  dpc(rgb0 << 4, rgb1 << 4, rgb2 << 4);
}

auto CPU::GTE::gpf() -> void {
  print("GPF\n");
}

auto CPU::GTE::gpl() -> void {
  print("GPL\n");
}

auto CPU::GTE::intpl() -> void {
  i16 i1 = ir1;
  i16 i2 = ir2;
  i16 i3 = ir3;

  setMacAndIr<1>((i64(rfc) << 12) - (i64(i1) << 12));
  setMacAndIr<2>((i64(gfc) << 12) - (i64(i2) << 12));
  setMacAndIr<3>((i64(bfc) << 12) - (i64(i3) << 12));

  setMacAndIr<1>((i64(ir0) << 12) + ir1 * i1, lm);
  setMacAndIr<2>((i64(ir0) << 12) + ir2 * i2, lm);
  setMacAndIr<3>((i64(ir0) << 12) + ir3 * i3, lm);

  pushColor();
}

auto CPU::GTE::mvmva() -> void {
  i32 tx, ty, tz;
  switch(tv) {
  case 0: tx = trx; ty = trY; tz = trz; break;
  case 1: tx = rbk; ty = gbk; tz = bbk; break;
  case 2: tx = rfc; ty = gfc; tz = bfc; break;
  case 3: tx = 0;   ty = 0;   tz = 0;   break;
  }

  i16 vx, vy, vz;
  switch(mv) {
  case 0: vx = vx0; vy = vy0; vz = vz0; break;
  case 1: vx = vx1; vy = vy1; vz = vz1; break;
  case 2: vx = vx2; vy = vy2; vz = vz2; break;
  case 3: vx = ir1; vy = ir2; vz = ir3; break;
  }

  i16 m11, m12, m13;
  i16 m21, m22, m23;
  i16 m31, m32, m33;
  switch(mm) {
  case 0:  //rotation
    m11 = rt11; m12 = rt12; m13 = rt13;
    m21 = rt21; m22 = rt22; m23 = rt23;
    m31 = rt31; m32 = rt32; m33 = rt33;
    break;
  case 1:  //light
    m11 = l11; m12 = l12; m13 = l13;
    m21 = l21; m22 = l22; m23 = l23;
    m31 = l31; m32 = l32; m33 = l33;
    break;
  case 2:  //color
    m11 = lr1; m12 = lr2; m13 = lr3;
    m21 = lg1; m22 = lg2; m23 = lg3;
    m31 = lb1; m32 = lb2; m33 = lb3;
    break;
  case 3:  //reserved
    m11 = -R;   m12 = +R;   m13 = ir0;
    m21 = rt13; m22 = rt13; m23 = rt13;
    m31 = rt22; m32 = rt22; m33 = rt22;
    break;
  }

  i64 x, y, z;
  if(tv != 2) {
    x = extend<1>(extend<1>(extend<1>((i64(tx) << 12) + m11 * vx) + m12 * vy) + m13 * vz);
    y = extend<2>(extend<2>(extend<2>((i64(ty) << 12) + m21 * vx) + m22 * vy) + m23 * vz);
    z = extend<3>(extend<3>(extend<3>((i64(tz) << 12) + m31 * vx) + m32 * vy) + m33 * vz);
  } else {
    setIr<1>(extend<1>((i64(tx) << 12) + m11 * vx) >> sf);
    setIr<2>(extend<2>((i64(ty) << 12) + m21 * vx) >> sf);
    setIr<3>(extend<3>((i64(tz) << 12) + m31 * vx) >> sf);

    x = extend<1>(extend<1>(m12 * vy) + m13 * vz);
    y = extend<1>(extend<1>(m22 * vy) + m23 * vz);
    z = extend<1>(extend<1>(m32 * vy) + m33 * vz);
  }

  setMacAndIr<1>(x, lm);
  setMacAndIr<2>(y, lm);
  setMacAndIr<3>(z, lm);
}

template<uint m>
auto CPU::GTE::nc(i32 vx, i32 vy, i32 vz) -> void {
  i64 x = extend<1>(extend<1>(extend<1>(l11 * vx) + l12 * vy) + l13 * vz);
  i64 y = extend<2>(extend<2>(extend<2>(l21 * vx) + l22 * vy) + l23 * vz);
  i64 z = extend<3>(extend<3>(extend<3>(l31 * vx) + l32 * vy) + l33 * vz);

  setMacAndIr<1>(x, lm);
  setMacAndIr<2>(y, lm);
  setMacAndIr<3>(z, lm);

  x = extend<1>(extend<1>(extend<1>((i64(rbk) << 12) + lr1 * ir1) + lr2 * ir2) + lr3 * ir3);
  y = extend<2>(extend<2>(extend<2>((i64(gbk) << 12) + lg1 * ir1) + lg2 * ir2) + lg3 * ir3);
  z = extend<3>(extend<3>(extend<3>((i64(bbk) << 12) + lb1 * ir1) + lb2 * ir2) + lb3 * ir3);

  setMacAndIr<1>(x, lm);
  setMacAndIr<2>(y, lm);
  setMacAndIr<3>(z, lm);

  if constexpr(m == 1) {
    setMacAndIr<1>(R * ir1);
    setMacAndIr<2>(G * ir2);
    setMacAndIr<3>(B * ir3);
  }

  if constexpr(m == 2) {
    i16 i1 = ir1;
    i16 i2 = ir2;
    i16 i3 = ir3;

    setMacAndIr<1>((i64(rfc) << 12) - R * ir1);
    setMacAndIr<2>((i64(gfc) << 12) - G * ir2);
    setMacAndIr<3>((i64(bfc) << 12) - B * ir3);

    setMacAndIr<1>(R * i1 + ir0 * ir1, lm);
    setMacAndIr<2>(G * i2 + ir0 * ir2, lm);
    setMacAndIr<3>(B * i3 + ir0 * ir3, lm);
  }

  pushColor();
}

auto CPU::GTE::nccs() -> void {
  nc<1>(vx0, vy0, vz0);
}

auto CPU::GTE::ncct() -> void {
  nc<1>(vx0, vy0, vz0);
  nc<1>(vx1, vy1, vz1);
  nc<1>(vx2, vy2, vz2);
}

auto CPU::GTE::ncds() -> void {
  nc<2>(vx0, vy0, vz0);
}

auto CPU::GTE::ncdt() -> void {
  nc<2>(vx0, vy0, vz0);
  nc<2>(vx1, vy1, vz1);
  nc<2>(vx2, vy2, vz2);
}

auto CPU::GTE::nclip() -> void {
  i64 p0 = i64(sx0) * i64(sy1);
  i64 p1 = i64(sx1) * i64(sy2);
  i64 p2 = i64(sx2) * i64(sy0);
  i64 p3 = i64(sx0) * i64(sy2);
  i64 p4 = i64(sx1) * i64(sy0);
  i64 p5 = i64(sx2) * i64(sy1);
  setMac<0>(p0 + p1 + p2 - p3 - p4 - p5);
}

auto CPU::GTE::ncs() -> void {
  nc<0>(vx0, vy0, vz0);
}

auto CPU::GTE::nct() -> void {
  nc<0>(vx0, vy0, vz0);
  nc<0>(vx1, vy1, vz1);
  nc<0>(vx2, vy2, vz2);
}

auto CPU::GTE::op() -> void {
  print("OP\n");
}

//rotation, translation, and perspective transformation
auto CPU::GTE::rtp(i32 vx, i32 vy, i32 vz, bool last) -> void {
  i64 x = extend<1>(extend<1>(extend<1>((i64(trx) << 12) + rt11 * vx) + rt12 * vy) + rt13 * vz);
  i64 y = extend<2>(extend<2>(extend<2>((i64(trY) << 12) + rt21 * vx) + rt22 * vy) + rt23 * vz);
  i64 z = extend<3>(extend<3>(extend<3>((i64(trz) << 12) + rt31 * vx) + rt32 * vy) + rt33 * vz);

  setMacAndIr<1>(x, lm);
  setMacAndIr<2>(y, lm);
  setMac<3>(z);
  saturate<3>(z >> 12);
  ir3 = std::clamp(mac3, lm ? 0 : -0x8000, +0x7fff);

  pushSZ(z >> 12);
  i64 dv = divide(h, sz3);
  i32 sx = setMac<0>(dv * ir1 + ofx);
  i32 sy = setMac<0>(dv * ir2 + ofy);
  pushSX(sx >> 16);
  pushSY(sy >> 16);
  if(!last) return;

  i64 sz = setMac<0>(dv * dqa + dqb);
  ir0 = saturate<0>(sz >> 12);
}

auto CPU::GTE::rtps() -> void {
  rtp(vx0, vy0, vz0, 1);
}

auto CPU::GTE::rtpt() -> void {
  rtp(vx0, vy0, vz0, 0);
  rtp(vx1, vy1, vz1, 0);
  rtp(vx2, vy2, vz2, 1);
}

auto CPU::GTE::sqr() -> void {
  print("SQR\n");
}

#undef R
#undef G
#undef B
