template<u32 Size>
auto SH2::Cache::read(u32 address) -> u32 {
  auto read = [&](auto& line) -> u32 {
    if constexpr(Size == Byte) { return        (line.bytes[address >> 0 & 15]); }
    if constexpr(Size == Word) { return bswap16(line.words[address >> 1 &  7]); }
    if constexpr(Size == Long) { return bswap32(line.longs[address >> 2 &  3]); }
  };

  auto entry = address >> 4 & 63;
  auto tag = address >> 10 & 0x7ffff;
  auto lru = lrus[entry];
  if(tags[Way3 | entry] == tag) { lrus[entry] = lruUpdate[3][lru]; return read(lines[Way3 | entry]); }
  if(tags[Way2 | entry] == tag) { lrus[entry] = lruUpdate[2][lru]; return read(lines[Way2 | entry]); }
  if(tags[Way1 | entry] == tag) { lrus[entry] = lruUpdate[1][lru]; return read(lines[Way1 | entry]); }
  if(tags[Way0 | entry] == tag) { lrus[entry] = lruUpdate[0][lru]; return read(lines[Way0 | entry]); }

  if((disableCode && address == self->PC - 4)
  || (disableData && address != self->PC - 4)
  ) {
    if constexpr(Size == Byte) { return self->busReadByte(address & 0x1fff'ffff); }
    if constexpr(Size == Word) { return self->busReadWord(address & 0x1fff'fffe); }
    if constexpr(Size == Long) { return self->busReadLong(address & 0x1fff'fffc); }
  }

  auto way = lruSelect[lru] | twoWay;
  auto index = way << 6 | entry;
  lrus[entry] = lruUpdate[way][lru];
  tags[index] = tag;  //marks line as valid
  lines[index].longs[0] = bswap32(self->busReadLong(address & 0x1fff'fff0 | 0x0));
  lines[index].longs[1] = bswap32(self->busReadLong(address & 0x1fff'fff0 | 0x4));
  lines[index].longs[2] = bswap32(self->busReadLong(address & 0x1fff'fff0 | 0x8));
  lines[index].longs[3] = bswap32(self->busReadLong(address & 0x1fff'fff0 | 0xc));
  self->step(12);
  return read(lines[index]);
}

template<u32 Size>
auto SH2::Cache::write(u32 address, u32 data) -> void {
  auto write = [&](auto& line) {
    if constexpr(Size == Byte) { line.bytes[address >> 0 & 15] =        (data); }
    if constexpr(Size == Word) { line.words[address >> 1 &  7] = bswap16(data); }
    if constexpr(Size == Long) { line.longs[address >> 2 &  3] = bswap32(data); }
  };

  auto entry = address >> 4 & 63;
  auto tag = address >> 10 & 0x7ffff;
  auto lru = lrus[entry];
  if(tags[Way3 | entry] == tag) { lrus[entry] = lruUpdate[3][lru]; return write(lines[Way3 | entry]); }
  if(tags[Way2 | entry] == tag) { lrus[entry] = lruUpdate[2][lru]; return write(lines[Way2 | entry]); }
  if(tags[Way1 | entry] == tag) { lrus[entry] = lruUpdate[1][lru]; return write(lines[Way1 | entry]); }
  if(tags[Way0 | entry] == tag) { lrus[entry] = lruUpdate[0][lru]; return write(lines[Way0 | entry]); }
}

template<u32 Size>
auto SH2::Cache::readData(u32 address) -> u32 {
  auto& line = lines[address >> 4 & 255];
  if constexpr(Size == Byte) { return        (line.bytes[address >> 0 & 15]); }
  if constexpr(Size == Word) { return bswap16(line.words[address >> 1 &  7]); }
  if constexpr(Size == Long) { return bswap32(line.longs[address >> 2 &  3]); }
}

template<u32 Size>
auto SH2::Cache::writeData(u32 address, u32 data) -> void {
  auto& line = lines[address >> 4 & 255];
  if constexpr(Size == Byte) { line.bytes[address >> 0 & 15] =        (data); }
  if constexpr(Size == Word) { line.words[address >> 1 &  7] = bswap16(data); }
  if constexpr(Size == Long) { line.longs[address >> 2 &  3] = bswap32(data); }
}

auto SH2::Cache::readAddress(u32 address) -> u32 {
  auto entry = address >> 4 & 63;
  auto tag = tags[waySelect << 6 | entry];
  return (tag & 0x7ffff << 10) | lrus[entry] << 4 | !bool(tag & Invalid) << 1;
}

auto SH2::Cache::writeAddress(u32 address, u32 data) -> void {
  auto invalid = address >> 2 & 1 ^ 1;
  auto entry = address >> 4 & 63;
  auto tag = address >> 10 & 0x7ffff;
  lrus[entry] = data >> 6 & 63;
  tags[waySelect << 6 | entry] = tag | invalid << 19;
}

auto SH2::Cache::purge(u32 address) -> void {
  auto entry = address >> 4 & 63;
  auto tag = address >> 10 & 0x7ffff;
  if(tags[Way0 | entry] == tag) tags[Way0 | entry] |= Invalid;
  if(tags[Way1 | entry] == tag) tags[Way1 | entry] |= Invalid;
  if(tags[Way2 | entry] == tag) tags[Way2 | entry] |= Invalid;
  if(tags[Way3 | entry] == tag) tags[Way3 | entry] |= Invalid;
}

template<u32 Ways>
auto SH2::Cache::purge() -> void {
  for(auto index : range(64)) lrus[index] = 0;
  for(auto index : range(64 * Ways)) tags[index] |= Invalid;
}

auto SH2::Cache::power() -> void {
  purge(4 * 256);

  for(n6 n : range(64)) {
    if(n.bit(5) == 1 && n.bit(4) == 1 && n.bit(3) == 1) { lruSelect[n] = 0; continue; }
    if(n.bit(5) == 0 && n.bit(2) == 1 && n.bit(1) == 1) { lruSelect[n] = 1; continue; }
    if(n.bit(4) == 0 && n.bit(2) == 0 && n.bit(0) == 1) { lruSelect[n] = 2; continue; }
    if(n.bit(3) == 0 && n.bit(1) == 0 && n.bit(0) == 0) { lruSelect[n] = 3; continue; }
    lruSelect[n] = 3;  //50% of entries will not match any rule: fallback to way 3
  }

  //way 0
  for(n6 n : range(64)) {
    n6 v = n;
    v.bit(5) = 0;
    v.bit(4) = 0;
    v.bit(3) = 0;
    lruUpdate[0][n] = v;
  }

  //way 1
  for(n6 n : range(64)) {
    n6 v = n;
    v.bit(5) = 1;
    v.bit(2) = 0;
    v.bit(1) = 0;
    lruUpdate[1][n] = v;
  }

  //way 2
  for(n6 n : range(64)) {
    n6 v = n;
    v.bit(4) = 1;
    v.bit(2) = 1;
    v.bit(0) = 0;
    lruUpdate[2][n] = v;
  }

  //way 3
  for(n6 n : range(64)) {
    n6 v = n;
    v.bit(3) = 1;
    v.bit(1) = 1;
    v.bit(0) = 1;
    lruUpdate[3][n] = v;
  }
}
