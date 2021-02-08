template<u32 Size> auto M68K::read(DataRegister reg) -> n32 {
  return clip<Size>(r.d[reg.number]);
}

template<u32 Size> auto M68K::write(DataRegister reg, n32 data) -> void {
  r.d[reg.number] = (r.d[reg.number] & ~mask<Size>()) | (data & mask<Size>());
}

//

template<u32 Size> auto M68K::read(AddressRegister reg) -> n32 {
  return sign<Size>(r.a[reg.number]);
}

template<u32 Size> auto M68K::write(AddressRegister reg, n32 data) -> void {
  r.a[reg.number] = sign<Size>(data);
}

//

//CCR,SR unused bits cannot be set; always read out as 0

auto M68K::readCCR() -> n8 {
  return r.c << 0 | r.v << 1 | r.z << 2 | r.n << 3 | r.x << 4;
}

auto M68K::readSR() -> n16 {
  return readCCR() << 0 | r.i << 8 | r.s << 13 | r.t << 15;
}

auto M68K::writeCCR(n8 ccr) -> void {
  r.c = ccr.bit(0);
  r.v = ccr.bit(1);
  r.z = ccr.bit(2);
  r.n = ccr.bit(3);
  r.x = ccr.bit(4);
}

auto M68K::writeSR(n16 sr) -> void {
  writeCCR(sr);

  //when entering or exiting supervisor mode; swap SSP and USP into A7
  if(r.s != sr.bit(13)) swap(r.a[7], r.sp);

  r.i = sr.bit(8,10);
  r.s = sr.bit(13);
  r.t = sr.bit(15);
}
