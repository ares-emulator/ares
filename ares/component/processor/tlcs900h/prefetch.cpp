#define PC r.pc.l.l0

//called on branches, returns, and interrupts
auto TLCS900H::invalidate() -> void {
  if(PIC) step(PIC), PIC = 0;
  PIQ.flush();
}

auto TLCS900H::prefetch(u32 clocks) -> void {
  PIC += clocks;
  while(PIC) {
    if(PIQ.size() >= 3) break;
    auto address = PC + PIQ.size();
    auto size = width(address);
    auto wait = speed(size, address);
    if(wait > PIC) break;
    PIC -= wait;
    if(size == Byte || address & 1) {
      auto byte = read(Byte, address);
      PIQ.write(byte);
    } else {
      auto word = read(Word, address);
      PIQ.write(word >> 0);
      PIQ.write(word >> 8);
    }
  }
}

template<> auto TLCS900H::fetch<n8>() -> n8 {
  prefetch(0);
  if(PIQ.empty()) {
    auto address = PC + PIQ.size();
    auto size = width(address);
    if(size == Byte || address & 1) {
      auto byte = read(Byte, address);
      PIQ.write(byte);
    } else {
      auto word = read(Word, address);
      PIQ.write(word >> 0);
      PIQ.write(word >> 8);
    }
  }
  return PC++, PIQ.read(0);
}

template<> auto TLCS900H::fetch<n16>() -> n16 {
  u8 d0 = fetch<n8>();
  u8 d1 = fetch<n8>();
  return d0 << 0 | d1 << 8;
}

template<> auto TLCS900H::fetch<n24>() -> n24 {
  u8 d0 = fetch<n8>();
  u8 d1 = fetch<n8>();
  u8 d2 = fetch<n8>();
  return d0 << 0 | d1 << 8 | d2 << 16;
}

template<> auto TLCS900H::fetch<n32>() -> n32 {
  u8 d0 = fetch<n8>();
  u8 d1 = fetch<n8>();
  u8 d2 = fetch<n8>();
  u8 d3 = fetch<n8>();
  return d0 << 0 | d1 << 8 | d2 << 16 | d3 << 24;
}

#undef PC
