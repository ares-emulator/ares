auto V30MZ::read(Size size, n16 segment, n16 address) -> n32 {
  n32 data;
  if(size >= Byte) data.byte(0) = read(segment * 16 + address++);
  if(size >= Word) data.byte(1) = read(segment * 16 + address++);
  if(size >= Long) data.byte(2) = read(segment * 16 + address++);
  if(size >= Long) data.byte(3) = read(segment * 16 + address++);
  return data;
}

auto V30MZ::write(Size size, n16 segment, n16 address, n16 data) -> void {
  if(size >= Byte) write(segment * 16 + address++, data.byte(0));
  if(size >= Word) write(segment * 16 + address++, data.byte(1));
}

//

auto V30MZ::in(Size size, n16 address) -> n16 {
  n16 data;
  if(size >= Byte) data.byte(0) = in(address++);
  if(size >= Word) data.byte(1) = in(address++);
  return data;
}

auto V30MZ::out(Size size, n16 address, n16 data) -> void {
  if(size >= Byte) out(address++, data.byte(0));
  if(size >= Word) out(address++, data.byte(1));
}

//

auto V30MZ::fetch(Size size) -> n16 {
  wait(size);
  n16 data = read(size, r.cs, r.ip);
  return r.ip += size, data;
}

//

auto V30MZ::pop() -> n16 {
  n16 data = read(Word, r.ss, r.sp);
  return r.sp += Word, data;
}

auto V30MZ::push(n16 data) -> void {
  r.sp -= Word;
  write(Word, r.ss, r.sp, data);
}
