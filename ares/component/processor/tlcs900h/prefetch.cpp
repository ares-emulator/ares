#define PC r.pc.l.l0

//called on reset, branches, returns, and interrupts
auto TLCS900H::invalidate() -> void {
  PIQ.flush();
}

template<> auto TLCS900H::fetch<n8>() -> n8 {
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

auto TLCS900H::wait(u32 clocks) -> void {
  if(PIQ.full()) return step(clocks);
  auto address = PC + PIQ.size();
  auto size = width(address);
  if(size == Byte || address & 1) {
    auto wait = speed(Byte, address);
    if(wait > clocks) return step(clocks);
    auto byte = read(Byte, address);
    if(!PIQ.full()) PIQ.write(byte);
    if(clocks -= wait) TLCS900H::wait(clocks);
  } else {
    auto wait = speed(Word, address);
    if(wait > clocks) return step(clocks);
    auto word = read(Word, address);
    if(!PIQ.full()) PIQ.write(word >> 0);
    if(!PIQ.full()) PIQ.write(word >> 8);
    if(clocks -= wait) TLCS900H::wait(clocks);
  }
}

#undef PC
