auto V30MZ::repeat() -> n8 {
  for(auto prefix : prefixes) {
    if(prefix == RepeatWhileZeroLo) return prefix;
    if(prefix == RepeatWhileZeroHi) return prefix;
  }
  return {};
}

auto V30MZ::segment(n16 segment) -> n16 {
  for(auto prefix : prefixes) {
    if(prefix == SegmentOverrideES) return r.es;
    if(prefix == SegmentOverrideCS) return r.cs;
    if(prefix == SegmentOverrideSS) return r.ss;
    if(prefix == SegmentOverrideDS) return r.ds;
  }
  return segment;
}

auto V30MZ::getAcc(Size size) -> n32 {
  if(size == Byte) return r.al;
  if(size == Word) return r.ax;
  if(size == Long) return r.dx << 16 | r.ax;
  unreachable;
}

auto V30MZ::setAcc(Size size, n32 data) -> void {
  if(size == Byte) r.al = data;
  if(size == Word) r.ax = data;
  if(size == Long) r.ax = data, r.dx = data >> 16;
}
