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
  if(PFP & 1) step(1);
}

auto V30MZ::prefetch() -> void {
  step(1);
  switch(PFP & 1) {
  case 0: if(PF.full()) break; PF.write(read(PS * 16 + PFP++));  //fallthrough
  case 1: if(PF.full()) break; PF.write(read(PS * 16 + PFP++));
  }
}

template<> auto V30MZ::fetch<Byte>() -> u16 {
  PC++;
  if(PF.empty()) prefetch();
  return PF.read(0);
}

template<> auto V30MZ::fetch<Word>() -> u16 {
  u8 lo = fetch<Byte>();
  u8 hi = fetch<Byte>();
  return lo << 0 | hi << 8;
}
