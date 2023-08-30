template<> auto V30MZ::read<Byte>(u16 segment, u16 address) -> u32 {
  step(speed(segment * 16 + address));
  u32 data = 0;
  data |= read(segment * 16 + address++) << 0;
  return data;
}

template<> auto V30MZ::read<Word>(u16 segment, u16 address) -> u32 {
  u32 data = 0;
  step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) << 0;
  if(width(segment * 16 + address) == Byte || !(address & 1)) step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) << 8;
  return data;
}

template<> auto V30MZ::read<Long>(u16 segment, u16 address) -> u32 {
  u32 data = 0;
  step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) <<  0;
  if(width(segment * 16 + address) == Byte || !(address & 1)) step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) <<  8;
  if(width(segment * 16 + address) == Byte || !(address & 1)) step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) << 16;
  if(width(segment * 16 + address) == Byte || !(address & 1)) step(speed(segment * 16 + address));
  data |= read(segment * 16 + address++) << 24;
  return data;
}

template<> auto V30MZ::write<Byte>(u16 segment, u16 address, u16 data) -> void {
  step(speed(segment * 16 + address));
  write(segment * 16 + address++, data >> 0);
}

template<> auto V30MZ::write<Word>(u16 segment, u16 address, u16 data) -> void {
  step(speed(segment * 16 + address));
  write(segment * 16 + address++, data >> 0);
  if(width(segment * 16 + address) == Byte || !(address & 1)) step(speed(segment * 16 + address));
  write(segment * 16 + address++, data >> 8);
}

template<> auto V30MZ::in<Byte>(u16 address) -> u16 {
  step(ioSpeed(address));
  u16 data = 0;
  data |= in(address++) << 0;
  return data;
}

template<> auto V30MZ::in<Word>(u16 address) -> u16 {
  u16 data = 0;
  step(ioSpeed(address));
  data |= in(address++) << 0;
  if(ioWidth(address) == Byte || !(address & 1)) step(ioSpeed(address));
  data |= in(address++) << 8;
  return data;
}

template<> auto V30MZ::out<Byte>(u16 address, u16 data) -> void {
  step(ioSpeed(address));
  out(address++, data >> 0);
}

template<> auto V30MZ::out<Word>(u16 address, u16 data) -> void {
  step(ioSpeed(address));
  out(address++, data >> 0);
  if(ioWidth(address) == Byte || !(address & 1)) step(ioSpeed(address));
  out(address++, data >> 8);
}

auto V30MZ::pop() -> u16 {
  u16 data = read<Word>(SS, SP);
  SP += Word;
  return data;
}

auto V30MZ::push(u16 data) -> void {
  SP -= Word;
  write<Word>(SS, SP, data);
}
