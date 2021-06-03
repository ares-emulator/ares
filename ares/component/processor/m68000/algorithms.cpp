template<u32 Size, bool extend> auto M68000::ADD(n32 source, n32 target) -> n32 {
  target = clip<Size>(target);
  source = clip<Size>(source);
  u32 result   = target + source + (extend ? r.x : 0);
  u32 carries  = target ^ source ^ result;
  u32 overflow = (target ^ result) & (source ^ result);

  r.c = (carries ^ overflow) & msb<Size>();
  r.v = overflow & msb<Size>();
  r.z = clip<Size>(result) ? 0 : (extend ? r.z : 1);
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return clip<Size>(result);
}

template<u32 Size> auto M68000::AND(n32 source, n32 target) -> n32 {
  u32 result = target & source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68000::ASL(n32 result, u32 shift) -> n32 {
  bool carry = false;
  u32 overflow = 0;
  for(auto _ : range(shift)) {
    carry = result & msb<Size>();
    u32 before = result;
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

template<u32 Size> auto M68000::ASR(n32 result, u32 shift) -> n32 {
  bool carry = false;
  u32 overflow = 0;
  for(auto _ : range(shift)) {
    carry = result & lsb<Size>();
    u32 before = result;
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

template<u32 Size> auto M68000::CMP(n32 source, n32 target) -> n32 {
  target = clip<Size>(target);
  source = clip<Size>(source);
  u32 result   = target - source;
  u32 carries  = target ^ source ^ result;
  u32 overflow = (target ^ result) & (source ^ target);

  r.c = (carries ^ overflow) & msb<Size>();
  r.v = overflow & msb<Size>();
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68000::EOR(n32 source, n32 target) -> n32 {
  u32 result = target ^ source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68000::LSL(n32 result, u32 shift) -> n32 {
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

template<u32 Size> auto M68000::LSR(n32 result, u32 shift) -> n32 {
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

template<u32 Size> auto M68000::OR(n32 source, n32 target) -> n32 {
  u32 result = target | source;

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;

  return clip<Size>(result);
}

template<u32 Size> auto M68000::ROL(n32 result, u32 shift) -> n32 {
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

template<u32 Size> auto M68000::ROR(n32 result, u32 shift) -> n32 {
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

template<u32 Size> auto M68000::ROXL(n32 result, u32 shift) -> n32 {
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

template<u32 Size> auto M68000::ROXR(n32 result, u32 shift) -> n32 {
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

template<u32 Size, bool extend> auto M68000::SUB(n32 source, n32 target) -> n32 {
  target = clip<Size>(target);
  source = clip<Size>(source);
  u32 result   = target - source - (extend ? r.x : 0);
  u32 carries  = target ^ source ^ result;
  u32 overflow = (target ^ result) & (source ^ target);

  r.c = (carries ^ overflow) & msb<Size>();
  r.v = overflow & msb<Size>();
  r.z = clip<Size>(result) ? 0 : (extend ? r.z : 1);
  r.n = sign<Size>(result) < 0;
  r.x = r.c;

  return result;
}
