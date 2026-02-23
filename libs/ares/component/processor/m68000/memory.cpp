//these functions transform internal accesses to bus accesses, and handle conversions:
//* A1-A23 are passed to the bus
//* A0 in word mode is ignored, and drives both UDS and LDS
//* A0 in byte mode drives either UDS or LDS
//* upper = /UDS (1 = selected; eg /UDS is inverted)
//* lower = /LDS (1 = selected; eg /LDS is inverted)
//* /UDS is where A0=0 and maps to D8-D15
//* /LDS is where A0=1 and maps to D0-D7

template<> auto M68000::read<Byte>(n32 address) -> n32 {
  wait(4);
  if(address & 1) {
    return read(0, 1, address & ~1).byte(0);  /* /LDS */
  } else {
    return read(1, 0, address & ~1).byte(1);  /* /UDS */
  }
}

template<> auto M68000::read<Word>(n32 address) -> n32 {
  wait(4);
  return read(1, 1, address & ~1);
}

template<> auto M68000::read<Long>(n32 address) -> n32 {
  wait(4);
  n32 data    = read(1, 1, address + 0 & ~1) << 16;
  wait(4);
  return data | read(1, 1, address + 2 & ~1) <<  0;
}

//

template<> auto M68000::write<Byte>(n32 address, n32 data) -> void {
  wait(4);
  if(address & 1) {
    return write(0, 1, address & ~1, data << 8 | (n8)data << 0);  /* /LDS */
  } else {
    return write(1, 0, address & ~1, data << 8 | (n8)data << 0);  /* /UDS */
  }
}

template<> auto M68000::write<Word>(n32 address, n32 data) -> void {
  wait(4);
  return write(1, 1, address & ~1, data);
}

template<> auto M68000::write<Long>(n32 address, n32 data) -> void {
  wait(4);
  write(1, 1, address + 0 & ~1, data >> 16);
  wait(4);
  write(1, 1, address + 2 & ~1, data >>  0);
}

//

template<> auto M68000::write<Byte, Reverse>(n32 address, n32 data) -> void {
  wait(4);
  if(address & 1) {
    return write(0, 1, address & ~1, data << 8 | (n8)data << 0);  /* /LDS */
  } else {
    return write(1, 0, address & ~1, data << 8 | (n8)data << 0);  /* /UDS */
  }
}

template<> auto M68000::write<Word, Reverse>(n32 address, n32 data) -> void {
  wait(4);
  return write(1, 1, address & ~1, data);
}

template<> auto M68000::write<Long, Reverse>(n32 address, n32 data) -> void {
  wait(4);
  write(1, 1, address + 2 & ~1, data >>  0);
  wait(4);
  write(1, 1, address + 0 & ~1, data >> 16);
}

//

template<> auto M68000::extension<Byte>() -> n32 {
  wait(4);
  r.ir  = r.irc;
  r.irc = read(1, 1, r.pc & ~1);
  r.pc += 2;
  return (n8)r.ir;
}

template<> auto M68000::extension<Word>() -> n32 {
  wait(4);
  r.ir  = r.irc;
  r.irc = read(1, 1, r.pc & ~1);
  r.pc += 2;
  return r.ir;
}

template<> auto M68000::extension<Long>() -> n32 {
  auto hi = extension<Word>();
  auto lo = extension<Word>();
  return hi << 16 | lo << 0;
}

//

auto M68000::prefetch() -> n16 {
  wait(4);
  r.ir  = r.irc;
  r.irc = read(1, 1, r.pc & ~1);
  r.pc += 2;
  return r.ir;
}

//take the prefetched value without reloading the prefetch.
//this is used by instructions such as JMP and JSR.

auto M68000::prefetched() -> n16 {
  r.ir  = r.irc;
  r.irc = 0x0000;
  r.pc += 2;
  return r.ir;
}

//

template<u32 Size> auto M68000::pop() -> n32 {
  auto data = read<Size>((n32)r.a[7]);
  r.a[7] += bytes<Size>();
  return data;
}

//

template<u32 Size> auto M68000::push(n32 data) -> void {
  r.a[7] -= bytes<Size>();
  return write<Size, Reverse>((n32)r.a[7], data);
}
