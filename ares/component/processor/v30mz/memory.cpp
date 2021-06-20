template<> auto V30MZ::read<Byte>(u16 segment, u16 address) -> u32 {
  step(1);
  u32 data = 0;
  data |= read(segment * 16 + address++) << 0;
  return data;
}

template<> auto V30MZ::read<Word>(u16 segment, u16 address) -> u32 {
  step(1 + (address & 1));
  u32 data = 0;
  data |= read(segment * 16 + address++) << 0;
  data |= read(segment * 16 + address++) << 8;
  return data;
}

template<> auto V30MZ::read<Long>(u16 segment, u16 address) -> u32 {
  step(2 + (address & 1));
  u32 data = 0;
  data |= read(segment * 16 + address++) <<  0;
  data |= read(segment * 16 + address++) <<  8;
  data |= read(segment * 16 + address++) << 16;
  data |= read(segment * 16 + address++) << 24;
  return data;
}

template<> auto V30MZ::write<Byte>(u16 segment, u16 address, u16 data) -> void {
  step(1);
  write(segment * 16 + address++, data >> 0);
}

template<> auto V30MZ::write<Word>(u16 segment, u16 address, u16 data) -> void {
  step(1 + (address & 1));
  write(segment * 16 + address++, data >> 0);
  write(segment * 16 + address++, data >> 8);
}

template<> auto V30MZ::in<Byte>(u16 address) -> u16 {
  step(1);
  u16 data = 0;
  data |= in(address++) << 0;
  return data;
}

template<> auto V30MZ::in<Word>(u16 address) -> u16 {
  step(1 + (address & 1));
  u16 data = 0;
  data |= in(address++) << 0;
  data |= in(address++) << 8;
  return data;
}

template<> auto V30MZ::out<Byte>(u16 address, u16 data) -> void {
  step(1);
  out(address++, data >> 0);
}

template<> auto V30MZ::out<Word>(u16 address, u16 data) -> void {
  step(1 + (address & 1));
  out(address++, data >> 0);
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
