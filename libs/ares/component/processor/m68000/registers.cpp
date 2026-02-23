template<u32 Size> auto M68000::read(DataRegister reg) -> n32 {
  if constexpr(Size == Byte) return (u8 )r.d[reg.number];
  if constexpr(Size == Word) return (u16)r.d[reg.number];
  if constexpr(Size == Long) return (u32)r.d[reg.number];
  unreachable;
}

template<u32 Size> auto M68000::write(DataRegister reg, n32 data) -> void {
  if constexpr(Size == Byte) r.d[reg.number] = r.d[reg.number] &   ~0xff | data &   0xff;
  if constexpr(Size == Word) r.d[reg.number] = r.d[reg.number] & ~0xffff | data & 0xffff;
  if constexpr(Size == Long) r.d[reg.number] = data;
}

//

template<u32 Size> auto M68000::read(AddressRegister reg) -> n32 {
  if constexpr(Size == Byte) return (s8 )r.a[reg.number];
  if constexpr(Size == Word) return (s16)r.a[reg.number];
  if constexpr(Size == Long) return (s32)r.a[reg.number];
  unreachable;
}

template<u32 Size> auto M68000::write(AddressRegister reg, n32 data) -> void {
  if constexpr(Size == Byte) r.a[reg.number] = (s8 )data;
  if constexpr(Size == Word) r.a[reg.number] = (s16)data;
  if constexpr(Size == Long) r.a[reg.number] = (s32)data;
}

//

//CCR/SR unused bits cannot be set; always read out as 0

auto M68000::readCCR() -> n8 {
  return r.c << 0 | r.v << 1 | r.z << 2 | r.n << 3 | r.x << 4;
}

auto M68000::readSR() -> n16 {
  return r.c << 0 | r.v << 1 | r.z << 2 | r.n << 3 | r.x << 4 | r.i << 8 | r.s << 13 | r.t << 15;
}

auto M68000::writeCCR(n8 ccr) -> void {
  r.c = ccr.bit(0);
  r.v = ccr.bit(1);
  r.z = ccr.bit(2);
  r.n = ccr.bit(3);
  r.x = ccr.bit(4);
}

auto M68000::writeSR(n16 sr) -> void {
  writeCCR(sr);

  //when entering or exiting supervisor mode; swap SSP and USP into A7
  if(r.s != sr.bit(13)) swap(r.a[7], r.sp);

  r.i = sr.bit(8,10);
  r.s = sr.bit(13);
  r.t = sr.bit(15);
}
