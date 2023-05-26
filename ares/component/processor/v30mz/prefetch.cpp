auto V30MZ::wait(u32 clocks) -> void {
  while(clocks--) prefetch();
}

auto V30MZ::loop() -> void {
  queue<u8[16]> pf;
  pf.write(opcode);
  while(!PF.empty()) {
    if(pf.full()) { PFP--; break; }
    pf.write(PF.read(0));
  }
  PF = pf;
}

auto V30MZ::flush() -> void {
  PF.flush();
  PFP = PC;
  PFW = 0;
}

auto V30MZ::prefetch() -> void {
  step(1);
  auto address = PS * 16 + PFP;
  auto size = width(address);
  auto wait = speed(address);
  if(++PFW < wait) return;
  PFW -= wait;
  if(size == Word && !(PFP & 1)) if(!PF.full()) PF.write(read(PS * 16 + PFP++));
  if(!PF.full()) PF.write(read(PS * 16 + PFP++));
}

template<> auto V30MZ::fetch<Byte>() -> u16 {
  PC++;
  while(PF.size() < 2) prefetch();
  return PF.read(0);
}

template<> auto V30MZ::fetch<Word>() -> u16 {
  PC += 2;
  while(PF.size() < 2) prefetch();
  u8 lo = PF.read(0);
  u8 hi = PF.read(0);
  return lo << 0 | hi << 8;
}
