#define PC r.pc.l.l0

//called on reset, branches, returns, and interrupts
auto TLCS900H::invalidate() -> void {
  p.valid = 0;
  p.data  = 0;
}

//called during CPU idle states
auto TLCS900H::prefetch() -> void {
  if(p.valid >= 3) return idle(2);

  auto address = PC + p.valid;
  auto size = width(address);
  if(size == Byte || address & 1) {
    auto byte = read(Byte, address);
    p.data |= byte << p.valid * 8;
    p.valid += 1;
  } else {
    auto word = read(Word, address);
    p.data |= word << p.valid * 8;
    p.valid += 2;
  }
}

#undef PC
