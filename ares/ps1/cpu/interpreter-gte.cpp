auto CPU::GTE::constructTable() -> void {
  for(u32 n : range(256)) {
    unsignedNewtonRaphsonTable[n] = max(0, (0x40000 / (n + 0x100) + 1) / 2 - 0x101);
  }
  unsignedNewtonRaphsonTable[256] = 0;
}

auto CPU::GTE::countLeadingZeroes16(u16 value) -> u32 {
  u32 zeroes = 0;
  while(!(value >> 15) && zeroes < 16) value <<= 1, zeroes++;
  return zeroes;
}

auto CPU::GTE::countLeadingZeroes32(u32 value) -> u32 {
  u32 zeroes = 0;
  if(!(value >> 31)) value = ~value;
  while(value >> 31) value <<= 1, zeroes++;
  return zeroes;
}

//

auto CPU::GTE::getDataRegister(u32 index) -> u32 {
  u32 data = 0;
  switch(index) {
  case  0: data = u16(v.a.x) << 0 | u16(v.a.y) << 16; break;
  case  1: data = s16(v.a.z); break;
  case  2: data = u16(v.b.x) << 0 | u16(v.b.y) << 16; break;
  case  3: data = s16(v.b.z); break;
  case  4: data = u16(v.c.x) << 0 | u16(v.c.y) << 16; break;
  case  5: data = s16(v.c.z); break;
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
  case 16: data = u16(screen[0].z); break;
  case 17: data = u16(screen[1].z); break;
  case 18: data = u16(screen[2].z); break;
  case 19: data = u16(screen[3].z); break;
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
    u8 r = uclamp<5>(ir.x / 0x80);
    u8 g = uclamp<5>(ir.y / 0x80);
    u8 b = uclamp<5>(ir.z / 0x80);
    data = r << 0 | g << 5 | b << 10;
  } break;
  case 30: data = lzcs; break;
  case 31: data = lzcr; break;
  }
  return data;
}

auto CPU::GTE::setDataRegister(u32 index, u32 data) -> void {
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
  case 28:
    ir.r = (data >>  0 & 31) * 0x80;
    ir.g = (data >>  5 & 31) * 0x80;
    ir.b = (data >> 10 & 31) * 0x80;
    break;
  case 29:
    break;
  case 30:
    lzcs = data;
    lzcr = countLeadingZeroes32(lzcs);
    break;
  case 31:
    break;
  }
}

auto CPU::GTE::getControlRegister(u32 index) -> u32 {
  u32 data = 0;
  switch(index) {
  case  0: data = u16(rotation.a.x) << 0 | u16(rotation.a.y) << 16; break;
  case  1: data = u16(rotation.a.z) << 0 | u16(rotation.b.x) << 16; break;
  case  2: data = u16(rotation.b.y) << 0 | u16(rotation.b.z) << 16; break;
  case  3: data = u16(rotation.c.x) << 0 | u16(rotation.c.y) << 16; break;
  case  4: data = s16(rotation.c.z); break;
  case  5: data = translation.x; break;
  case  6: data = translation.y; break;
  case  7: data = translation.z; break;
  case  8: data = u16(light.a.x) << 0 | u16(light.a.y) << 16; break;
  case  9: data = u16(light.a.z) << 0 | u16(light.b.x) << 16; break;
  case 10: data = u16(light.b.y) << 0 | u16(light.b.z) << 16; break;
  case 11: data = u16(light.c.x) << 0 | u16(light.c.y) << 16; break;
  case 12: data = s16(light.c.z); break;
  case 13: data = backgroundColor.r; break;
  case 14: data = backgroundColor.g; break;
  case 15: data = backgroundColor.b; break;
  case 16: data = u16(color.a.x) << 0 | u16(color.a.y) << 16; break;
  case 17: data = u16(color.a.z) << 0 | u16(color.b.x) << 16; break;
  case 18: data = u16(color.b.y) << 0 | u16(color.b.z) << 16; break;
  case 19: data = u16(color.c.x) << 0 | u16(color.c.y) << 16; break;
  case 20: data = s16(color.c.z); break;
  case 21: data = farColor.r; break;
  case 22: data = farColor.g; break;
  case 23: data = farColor.b; break;
  case 24: data = ofx; break;
  case 25: data = ofy; break;
  case 26: data = s16(h); break;
  case 27: data = dqa; break;
  case 28: data = dqb; break;
  case 29: data = zsf3; break;
  case 30: data = zsf4; break;
  case 31: data = flag.value; break;
  }
  return data;
}

auto CPU::GTE::setControlRegister(u32 index, u32 data) -> void {
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
  case 31: flag.value = data & 0x7fff'f000; epilogue(); break;
  }
}

//

template<u32 id>
auto CPU::GTE::checkMac(s64 value) -> s64 {
  static constexpr s64 min = -(s64(1) << (id == 0 ? 31 : 43));
  static constexpr s64 max = +(s64(1) << (id == 0 ? 31 : 43)) - 1;
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

template<u32 id>
auto CPU::GTE::extend(s64 mac) -> s64 {
  checkMac<id>(mac);
  if constexpr(id == 1) return sclip<44>(mac);
  if constexpr(id == 2) return sclip<44>(mac);
  if constexpr(id == 3) return sclip<44>(mac);
  unreachable;
}

template<u32 id>
auto CPU::GTE::saturateIr(s32 value, bool lm) -> s32 {
  static constexpr s32 min = id == 0 ? 0x0000 : -0x8000;
  static constexpr s32 max = id == 0 ? 0x1000 : +0x7fff;
  s32 lmm = lm ? 0 : min;
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

template<u32 id>
auto CPU::GTE::saturateColor(s32 value) -> u8 {
  if(value < 0 || value > 255) {
    if constexpr(id == 0) flag.r_saturated = 1;
    if constexpr(id == 1) flag.g_saturated = 1;
    if constexpr(id == 2) flag.b_saturated = 1;
    return uclamp<8>(value);
  }
  return value;
}

//

template<u32 id>
auto CPU::GTE::setMac(s64 value) -> s64 {
  checkMac<id>(value);
  if constexpr(id == 0) { mac.t = value;       return value;       }
  if constexpr(id == 1) { mac.x = value >> sf; return value >> sf; }
  if constexpr(id == 2) { mac.y = value >> sf; return value >> sf; }
  if constexpr(id == 3) { mac.z = value >> sf; return value >> sf; }
}

template<u32 id>
auto CPU::GTE::setIr(s32 value, bool lm) -> void {
  if constexpr(id == 0) ir.t = saturateIr<0>(value, lm);
  if constexpr(id == 1) ir.x = saturateIr<1>(value, lm);
  if constexpr(id == 2) ir.y = saturateIr<2>(value, lm);
  if constexpr(id == 3) ir.z = saturateIr<3>(value, lm);
}

template<u32 id>
auto CPU::GTE::setMacAndIr(s64 value, bool lm) -> void {
  setIr<id>(setMac<id>(value), lm);
}

auto CPU::GTE::setMacAndIr(const v64& vector) -> void {
  setMacAndIr<1>(vector.x, lm);
  setMacAndIr<2>(vector.y, lm);
  setMacAndIr<3>(vector.z, lm);
}

auto CPU::GTE::setOtz(s64 value) -> void {
  static constexpr s64 min = 0x0000;
  static constexpr s64 max = 0xffff;
  value >>= 12;
  if(value < min) { otz = min; flag.otz_saturated = 1; return; }
  if(value > max) { otz = max; flag.otz_saturated = 1; return; }
  otz = value;
}

//

auto CPU::GTE::matrixMultiply(const m16& matrix, const v16& vector, const v32& translation) -> v64 {
  s64 x = extend<1>(extend<1>(extend<1>((s64(translation.x) << 12) + matrix.a.x * vector.x) + matrix.a.y * vector.y) + matrix.a.z * vector.z);
  s64 y = extend<2>(extend<2>(extend<2>((s64(translation.y) << 12) + matrix.b.x * vector.x) + matrix.b.y * vector.y) + matrix.b.z * vector.z);
  s64 z = extend<3>(extend<3>(extend<3>((s64(translation.z) << 12) + matrix.c.x * vector.x) + matrix.c.y * vector.y) + matrix.c.z * vector.z);
  return {x, y, z};
}

auto CPU::GTE::vectorMultiply(const v16& vector1, const v16& vector2, const v16& translation) -> v64 {
  s64 x = (s64(translation.x) << 12) + vector1.x * vector2.x;
  s64 y = (s64(translation.y) << 12) + vector1.y * vector2.y;
  s64 z = (s64(translation.z) << 12) + vector1.z * vector2.z;
  return {x, y, z};
}

auto CPU::GTE::vectorMultiply(const v16& vector, s16 scalar) -> v64 {
  s64 x = vector.x * scalar;
  s64 y = vector.y * scalar;
  s64 z = vector.z * scalar;
  return {x, y, z};
}

auto CPU::GTE::divide(u32 lhs, u32 rhs) -> u32 {
  if(rhs * 2 <= lhs) {
    flag.divide_overflow = 1;
    return 0x1ffff;
  }

  u32 shift = countLeadingZeroes16(rhs);
  lhs <<= shift;
  rhs <<= shift;

  u32 divisor = rhs | 0x8000;
  s32 x = s32(0x101 + unsignedNewtonRaphsonTable[(divisor & 0x7fff) + 0x40 >> 7]);
  s32 d = s32(divisor) * -x + 0x80 >> 8;
  u32 reciprocal = x * (0x20000 + d) + 0x80 >> 8;
  u32 result = u64(lhs) * reciprocal + 0x8000 >> 16;
  return min(0x1ffff, result);
}

auto CPU::GTE::pushScreenX(s32 sx) -> void {
  if(sx < -1024) sx = -1024, flag.sx2_saturated = 1;
  if(sx > +1023) sx = +1023, flag.sx2_saturated = 1;
  screen[0].x = screen[1].x;
  screen[1].x = screen[2].x;
  screen[2].x = sx;
}

auto CPU::GTE::pushScreenY(s32 sy) -> void {
  if(sy < -1024) sy = -1024, flag.sy2_saturated = 1;
  if(sy > +1023) sy = +1023, flag.sy2_saturated = 1;
  screen[0].y = screen[1].y;
  screen[1].y = screen[2].y;
  screen[2].y = sy;
}

auto CPU::GTE::pushScreenZ(s32 sz) -> void {
  if(sz < 0x0000) sz = 0x0000, flag.sz3_saturated = 1;
  if(sz > 0xffff) sz = 0xffff, flag.sz3_saturated = 1;
  screen[0].z = screen[1].z;
  screen[1].z = screen[2].z;
  screen[2].z = screen[3].z;
  screen[3].z = sz;
}

auto CPU::GTE::pushColor(s32 r, s32 g, s32 b) -> void {
  r = saturateColor<0>(r);
  g = saturateColor<1>(g);
  b = saturateColor<2>(b);
  rgb[0] = rgb[1];
  rgb[1] = rgb[2];
  rgb[2] = r << 0 | g << 8 | b << 16 | rgbc.t << 24;
}

auto CPU::GTE::pushColor() -> void {
  pushColor(mac.r >> 4, mac.g >> 4, mac.b >> 4);
}

//

inline auto CPU::GTE::prologue() -> void {
  flag.value = 0;
}

inline auto CPU::GTE::prologue(bool lm, u8 sf) -> void {
  this->lm = lm;
  this->sf = sf;
  flag.value = 0;
}

inline auto CPU::GTE::epilogue() -> void {
  flag.error = bool(flag.value & 0x7f87'e000);
}

//

#define v16 GTE::v16
#define v32 GTE::v32
#define v64 GTE::v64
#define m16 GTE::m16

#define getDataRegister gte.getDataRegister
#define setDataRegister gte.setDataRegister
#define getControlRegister gte.getControlRegister
#define setControlRegister gte.setControlRegister
#define checkMac gte.checkMac
#define extend gte.extend
#define saturateIr gte.saturateIr
#define setMac gte.setMac
#define setIr gte.setIr
#define setMacAndIr gte.setMacAndIr
#define setOtz gte.setOtz
#define matrixMultiply gte.matrixMultiply
#define vectorMultiply gte.vectorMultiply
#define divide gte.divide
#define pushScreenX gte.pushScreenX
#define pushScreenY gte.pushScreenY
#define pushScreenZ gte.pushScreenZ
#define pushColor gte.pushColor
#define prologue gte.prologue
#define epilogue gte.epilogue

#define v gte.v
#define rgbc gte.rgbc
#define ir gte.ir
#define screen gte.screen
#define rgb gte.rgb
#define mac gte.mac
#define rotation gte.rotation
#define translation gte.translation
#define light gte.light
#define backgroundColor gte.backgroundColor
#define color gte.color
#define farColor gte.farColor
#define ofx gte.ofx
#define ofy gte.ofy
#define h gte.h
#define dqa gte.dqa
#define dqb gte.dqb
#define zsf3 gte.zsf3
#define zsf4 gte.zsf4

auto CPU::AVSZ3() -> void {
  prologue();
  setOtz(setMac<0>(s64(zsf3) * (screen[1].z + screen[2].z + screen[3].z)));
  epilogue();
}

auto CPU::AVSZ4() -> void {
  prologue();
  setOtz(setMac<0>(s64(zsf4) * (screen[0].z + screen[1].z + screen[2].z + screen[3].z)));
  epilogue();
}

auto CPU::CC(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));
  setMacAndIr(vectorMultiply({rgbc.r << 4, rgbc.g << 4, rgbc.b << 4}, ir));
  pushColor();
  epilogue();
}

auto CPU::CDP(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));

  v16 i = ir;
  setMacAndIr<1>((s64(farColor.r) << 12) - ((rgbc.r << 4) * ir.x));
  setMacAndIr<2>((s64(farColor.g) << 12) - ((rgbc.g << 4) * ir.y));
  setMacAndIr<3>((s64(farColor.b) << 12) - ((rgbc.b << 4) * ir.z));

  setMacAndIr<1>(((rgbc.r << 4) * i.x) + ir.t * ir.x, lm);
  setMacAndIr<2>(((rgbc.g << 4) * i.y) + ir.t * ir.y, lm);
  setMacAndIr<3>(((rgbc.b << 4) * i.z) + ir.t * ir.z, lm);
  pushColor();
  epilogue();
}

auto CPU::CFC2(u32& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  load(rt, getControlRegister(rd));
}

auto CPU::CTC2(cu32& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  setControlRegister(rd, rt);
}

auto CPU::DCPL(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  v16 i = ir;
  v16 col = {rgbc.r << 4, rgbc.g << 4, rgbc.b << 4};
  setMacAndIr<1>((s64(farColor.r) << 12) - col.r * i.x);
  setMacAndIr<2>((s64(farColor.g) << 12) - col.g * i.y);
  setMacAndIr<3>((s64(farColor.b) << 12) - col.b * i.z);

  setMacAndIr<1>(col.r * i.x + ir.t * ir.x, lm);
  setMacAndIr<2>(col.g * i.y + ir.t * ir.y, lm);
  setMacAndIr<3>(col.b * i.z + ir.t * ir.z, lm);
  pushColor();
  epilogue();
}

//meta-instruction
auto CPU::DPC(const v16& col) -> void {
  setMacAndIr<1>((s64(farColor.r) << 12) - (col.r << 12));
  setMacAndIr<2>((s64(farColor.g) << 12) - (col.g << 12));
  setMacAndIr<3>((s64(farColor.b) << 12) - (col.b << 12));

  setMacAndIr(vectorMultiply({ir.t, ir.t, ir.t}, ir, col));
  pushColor();
}

auto CPU::DPCS(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  DPC({rgbc.r << 4, rgbc.g << 4, rgbc.b << 4});
  epilogue();
}

auto CPU::DPCT(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  DPC({u8(rgb[0] >> 0) << 4, u8(rgb[0] >> 8) << 4, u8(rgb[0] >> 16) << 4});
  DPC({u8(rgb[0] >> 0) << 4, u8(rgb[0] >> 8) << 4, u8(rgb[0] >> 16) << 4});
  DPC({u8(rgb[0] >> 0) << 4, u8(rgb[0] >> 8) << 4, u8(rgb[0] >> 16) << 4});
  epilogue();
}

auto CPU::GPF(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMacAndIr(vectorMultiply(ir, ir.t));
  pushColor();
  epilogue();
}

auto CPU::GPL(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMacAndIr<1>((s64(mac.x) << sf) + ir.t * ir.x, lm);
  setMacAndIr<2>((s64(mac.y) << sf) + ir.t * ir.y, lm);
  setMacAndIr<3>((s64(mac.z) << sf) + ir.t * ir.z, lm);
  pushColor();
  epilogue();
}

auto CPU::INTPL(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  v16 i = ir;
  setMacAndIr<1>((s64(farColor.r) << 12) - (i.x << 12));
  setMacAndIr<2>((s64(farColor.g) << 12) - (i.y << 12));
  setMacAndIr<3>((s64(farColor.b) << 12) - (i.z << 12));

  setMacAndIr<1>(s64(ir.t * ir.x) + (i.x << 12), lm);
  setMacAndIr<2>(s64(ir.t * ir.y) + (i.y << 12), lm);
  setMacAndIr<3>(s64(ir.t * ir.z) + (i.z << 12), lm);

  pushColor();
  epilogue();
}

auto CPU::LWC2(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  auto data = read<Word>(rs + imm);
  if(exception()) return;
  setDataRegister(rt, data);
}

auto CPU::MFC2(u32& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  load(rt, getDataRegister(rd));
}

auto CPU::MTC2(cu32& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  setDataRegister(rd, rt);
}

auto CPU::MVMVA(bool lm, u8 tv, u8 mv, u8 mm, u8 sf) -> void {
  prologue(lm, sf);
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
    setIr<1>(extend<1>((s64(tr.x) << 12) + matrix.a.x * vector.x) >> sf);
    setIr<2>(extend<2>((s64(tr.y) << 12) + matrix.b.x * vector.x) >> sf);
    setIr<3>(extend<3>((s64(tr.z) << 12) + matrix.c.x * vector.x) >> sf);

    v64 result;
    result.x = extend<1>(extend<1>(matrix.a.y * vector.y) + matrix.a.z * vector.z);
    result.y = extend<2>(extend<2>(matrix.b.y * vector.y) + matrix.b.z * vector.z);
    result.z = extend<3>(extend<3>(matrix.c.y * vector.y) + matrix.c.z * vector.z);
    setMacAndIr(result);
  }
  epilogue();
}

//meta-instruction
template<u32 m>
auto CPU::NC(const v16& vector) -> void {
  setMacAndIr(matrixMultiply(light, vector));
  setMacAndIr(matrixMultiply(color, ir, backgroundColor));

  if constexpr(m == 1) {
    setMacAndIr<1>((rgbc.r << 4) * ir.x);
    setMacAndIr<2>((rgbc.g << 4) * ir.y);
    setMacAndIr<3>((rgbc.b << 4) * ir.z);
  }

  if constexpr(m == 2) {
    v16 i = ir;
    setMacAndIr<1>((s64(farColor.r) << 12) - (rgbc.r << 4) * i.x);
    setMacAndIr<2>((s64(farColor.g) << 12) - (rgbc.g << 4) * i.y);
    setMacAndIr<3>((s64(farColor.b) << 12) - (rgbc.b << 4) * i.z);

    setMacAndIr<1>((rgbc.r << 4) * i.x + ir.t * ir.x, gte.lm);
    setMacAndIr<2>((rgbc.g << 4) * i.y + ir.t * ir.y, gte.lm);
    setMacAndIr<3>((rgbc.b << 4) * i.z + ir.t * ir.z, gte.lm);
  }

  pushColor();
}

auto CPU::NCCS(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<1>(v.a);
  epilogue();
}

auto CPU::NCCT(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<1>(v.a);
  NC<1>(v.b);
  NC<1>(v.c);
  epilogue();
}

auto CPU::NCDS(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<2>(v.a);
  epilogue();
}

auto CPU::NCDT(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<2>(v.a);
  NC<2>(v.b);
  NC<2>(v.c);
  epilogue();
}

auto CPU::NCLIP() -> void {
  prologue();
  s64 p0 = s64(screen[0].x) * s64(screen[1].y);
  s64 p1 = s64(screen[1].x) * s64(screen[2].y);
  s64 p2 = s64(screen[2].x) * s64(screen[0].y);
  s64 p3 = s64(screen[0].x) * s64(screen[2].y);
  s64 p4 = s64(screen[1].x) * s64(screen[0].y);
  s64 p5 = s64(screen[2].x) * s64(screen[1].y);
  setMac<0>(p0 + p1 + p2 - p3 - p4 - p5);
  epilogue();
}

auto CPU::NCS(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<0>(v.a);
  epilogue();
}

auto CPU::NCT(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  NC<0>(v.a);
  NC<0>(v.b);
  NC<0>(v.c);
  epilogue();
}

auto CPU::OP(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMac<1>(rotation.b.y * ir.z - rotation.c.z * ir.y);
  setMac<2>(rotation.c.z * ir.x - rotation.a.x * ir.z);
  setMac<3>(rotation.a.x * ir.y - rotation.b.y * ir.x);

  setIr<1>(mac.x, lm);
  setIr<2>(mac.y, lm);
  setIr<3>(mac.z, lm);
  epilogue();
}

//meta-instruction: rotation, translation, and perspective transformation
auto CPU::RTP(v16 vector, bool last) -> void {
  auto [x, y, z] = matrixMultiply(rotation, vector, translation);
  setMacAndIr<1>(x, gte.lm);
  setMacAndIr<2>(y, gte.lm);
  setMac<3>(z);
  saturateIr<3>(z >> 12);
  ir.z = std::clamp(mac.z, gte.lm ? 0x0000 : -0x8000, +0x7fff);

  pushScreenZ(z >> 12);
  s64 dv = divide(h, screen[3].z);
  s32 sx = checkMac<0>(s64(dv * ir.x * 1.0f) + ofx) >> 16;
  s32 sy = checkMac<0>(dv * ir.y + ofy) >> 16;
  pushScreenX(sx);
  pushScreenY(sy);
  if(!last) return;

  s64 sz = setMac<0>(dv * dqa + dqb);
  ir.t = saturateIr<0>(sz >> 12);
}

auto CPU::RTPS(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  RTP(v.a, 1);
  epilogue();
}

auto CPU::RTPT(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  RTP(v.a, 0);
  RTP(v.b, 0);
  RTP(v.c, 1);
  epilogue();
}

auto CPU::SQR(bool lm, u8 sf) -> void {
  prologue(lm, sf);
  setMacAndIr(vectorMultiply(ir, ir));
  epilogue();
}

auto CPU::SWC2(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor2) return exception.coprocessor();
  auto data = getDataRegister(rt);
  write<Word>(rs + imm, data);
}

#undef v16
#undef v32
#undef v64
#undef m16

#undef getDataRegister
#undef setDataRegister
#undef getControlRegister
#undef setControlRegister
#undef checkMac
#undef extend
#undef saturateIr
#undef setMac
#undef setIr
#undef setMacAndIr
#undef setOtz
#undef matrixMultiply
#undef vectorMultiply
#undef divide
#undef pushScreenX
#undef pushScreenY
#undef pushScreenZ
#undef pushColor
#undef prologue
#undef epilogue

#undef v
#undef rgbc
#undef ir
#undef screen
#undef rgb
#undef mac
#undef rotation
#undef translation
#undef light
#undef backgroundColor
#undef color
#undef farColor
#undef ofx
#undef ofy
#undef h
#undef dqa
#undef dqb
#undef zsf3
#undef zsf4
