auto SSP1601::fetch() -> u16 {
  return read(PC++);
}

auto SSP1601::readPR1(u16 op) -> u16 {
  return readPR1(op >> 0, op >> 8, op >> 2);
}

auto SSP1601::readPR1(n2 r, n1 bank, n2 mode) -> u16 {
  auto p = &RAM[bank << 8];
  auto i = &R[bank << 2];
  if(r == 3) return p[mode];
  if(mode == 0) return p[i[r]];
  if(mode == 1) return p[i[r]++];
  auto v = p[i[r]];
  auto mask = (1 << RPL) - 1;
  if(mode == 2) {
    if(!RPL) return i[r]--, v;
    i[r] = (i[r] & ~mask) | (i[r] - 1 & mask);
  }
  if(mode == 3) {
    if(!RPL) return i[r]++, v;
    i[r] = (i[r] & ~mask) | (i[r] + 1 & mask);
  }
  return v;
}

auto SSP1601::writePR1(u16 op, u16 v) -> void {
  n2 r    = op >> 0;
  n1 bank = op >> 8;
  n2 mode = op >> 2;
  auto p = &RAM[bank << 8];
  auto i = &R[bank << 2];
  if(r == 3) return (void)(p[mode] = v);
  if(mode == 0) return (void)(p[i[r]] = v);
  if(mode == 1) return (void)(p[i[r]++] = v);
  if(mode == 2) return (void)(p[i[r]--] = v);
  if(mode == 3) return (void)(p[i[r]++] = v);
}

auto SSP1601::readPR2(u16 op) -> u16 {
  n2 r    = op >> 0;
  n1 bank = op >> 8;
  n2 mode = op >> 2;
  auto p = &RAM[bank << 8];
  auto i = &R[bank << 2];
  if(r == 3) return read(p[mode]++);
  if(mode == 0) return read(p[i[r]]++);
  if(mode == 1) return 0;
  if(mode == 2) return 0;
  if(mode == 3) return 0;
  unreachable;
}
