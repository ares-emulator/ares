auto V30MZ::read(Size size, n16 segment, n16 address) -> n32 {
  step(size == Long ? 2 : 1);
  n32 data;
  if(size >= Byte) data.byte(0) = read(segment * 16 + address++);
  if(size >= Word) data.byte(1) = read(segment * 16 + address++);
  if(size >= Long) data.byte(2) = read(segment * 16 + address++);
  if(size >= Long) data.byte(3) = read(segment * 16 + address++);
  return data;
}

auto V30MZ::write(Size size, n16 segment, n16 address, n16 data) -> void {
  step(1);
  if(size >= Byte) write(segment * 16 + address++, data.byte(0));
  if(size >= Word) write(segment * 16 + address++, data.byte(1));
}

//

auto V30MZ::in(Size size, n16 address) -> n16 {
  step(1);
  n16 data;
  if(size >= Byte) data.byte(0) = in(address++);
  if(size >= Word) data.byte(1) = in(address++);
  return data;
}

auto V30MZ::out(Size size, n16 address, n16 data) -> void {
  step(1);
  if(size >= Byte) out(address++, data.byte(0));
  if(size >= Word) out(address++, data.byte(1));
}

//

auto V30MZ::loop() -> void {
  queue<u8[16]> pf;
  pf.write(opcode);
  while(!r.pf.empty()) {
    if(pf.full()) { r.pfp--; break; }
    pf.write(r.pf.read(0));
  }
  r.pf = pf;
}

auto V30MZ::flush() -> void {
  r.pf.flush();
  r.pfp = r.ip;
  if(r.pfp & 1) step(1);
}

auto V30MZ::wait(u32 clocks) -> void {
  while(clocks--) prefetch();
}

auto V30MZ::prefetch() -> void {
  step(1);
  switch(r.pfp & 1) {
  case 0: if(r.pf.full()) break; r.pf.write(read(r.cs * 16 + r.pfp++));  //fallthrough
  case 1: if(r.pf.full()) break; r.pf.write(read(r.cs * 16 + r.pfp++));
  }
}

auto V30MZ::fetch(Size size) -> n16 {
  n16 data;
  if(size >= Byte) {
    if(r.pf.empty()) prefetch();
    data.byte(0) = r.pf.read(0);
    r.ip++;
  }
  if(size >= Word) {
    if(r.pf.empty()) prefetch();
    data.byte(1) = r.pf.read(0);
    r.ip++;
  }
  return data;
}

//

auto V30MZ::pop() -> n16 {
  n16 data = read(Word, r.ss, r.sp);
  r.sp += Word;
  return data;
}

auto V30MZ::push(n16 data) -> void {
  r.sp -= Word;
  write(Word, r.ss, r.sp, data);
}
