template<u32 Size, bool extend> auto M68K::ADD(n32 source, n32 target) -> n32 {
  auto result = (n64)source + target;
  if(extend) result += r.x;

  r.c = sign<Size>(result >> 1) < 0;
  r.v = sign<Size>(~(target ^ source) & (target ^ result)) < 0;
  r.z = clip<Size>(result) ? 0 : (extend ? r.z : 1);
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::AND(n32 source, n32 target) -> n32 {
  n32 result = target & source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ASL(n32 result, u32 shift) -> n32 {
  bool carry = false;
  n32 overflow = 0;
  for(auto _ : range(shift)) {
    carry = result & msb<Size>();
    n32 before = result;
    result <<= 1;
    overflow |= before ^ result;
  }

  r.c = carry;
  r.v = sign<Size>(overflow) < 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  if(shift) r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ASR(n32 result, u32 shift) -> n32 {
  bool carry = false;
  n32 overflow = 0;
  for(auto _ : range(shift)) {
    carry = result & lsb<Size>();
    n32 before = result;
    result = sign<Size>(result) >> 1;
    overflow |= before ^ result;
  }

  r.c = carry;
  r.v = sign<Size>(overflow) < 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  if(shift) r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::CMP(n32 source, n32 target) -> n32 {
  auto result = (n64)target - source;

  r.c = sign<Size>(result >> 1) < 0;
  r.v = sign<Size>((target ^ source) & (target ^ result)) < 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::EOR(n32 source, n32 target) -> n32 {
  n32 result = target ^ source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::LSL(n32 result, u32 shift) -> n32 {
  bool carry = false;
  for(auto _ : range(shift)) {
    carry = result & msb<Size>();
    result <<= 1;
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  if(shift) r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::LSR(n32 result, u32 shift) -> n32 {
  bool carry = false;
  for(auto _ : range(shift)) {
    carry = result & lsb<Size>();
    result >>= 1;
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  if(shift) r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::OR(n32 source, n32 target) -> n32 {
  auto result = target | source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ROL(n32 result, u32 shift) -> n32 {
  bool carry = false;
  for(auto _ : range(shift)) {
    carry = result & msb<Size>();
    result = result << 1 | carry;
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ROR(n32 result, u32 shift) -> n32 {
  bool carry = false;
  for(auto _ : range(shift)) {
    carry = result & lsb<Size>();
    result >>= 1;
    if(carry) result |= msb<Size>();
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ROXL(n32 result, u32 shift) -> n32 {
  bool carry = r.x;
  for(auto _ : range(shift)) {
    bool extend = carry;
    carry = result & msb<Size>();
    result = result << 1 | extend;
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68K::ROXR(n32 result, u32 shift) -> n32 {
  bool carry = r.x;
  for(auto _ : range(shift)) {
    bool extend = carry;
    carry = result & lsb<Size>();
    result >>= 1;
    if(extend) result |= msb<Size>();
  }

  r.c = carry;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size, bool extend> auto M68K::SUB(n32 source, n32 target) -> n32 {
  auto result = (n64)target - source;
  if(extend) result -= r.x;

  r.c = sign<Size>(result >> 1) < 0;
  r.v = sign<Size>((target ^ source) & (target ^ result)) < 0;
  r.z = clip<Size>(result) ? 0 : (extend ? r.z : 1);
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return result;
}
