auto SH2::readByte(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    return cache[address & 0xfff];
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    return internalReadByte(address);
  }

  return busReadByte(address);
}

auto SH2::readWord(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    u16 data = 0;
    data |= cache[address & 0xffe | 0] << 8;
    data |= cache[address & 0xffe | 1] << 0;
    return data;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    u16 data = 0;
    data |= internalReadByte(address & ~1 | 0) << 8;
    data |= internalReadByte(address & ~1 | 1) << 0;
    return data;
  }

  return busReadWord(address & ~1);
}

auto SH2::readLong(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    u32 data = 0;
    data |= cache[address & 0xffc | 0] << 24;
    data |= cache[address & 0xffc | 1] << 16;
    data |= cache[address & 0xffc | 2] <<  8;
    data |= cache[address & 0xffc | 3] <<  0;
    return data;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    u32 data = 0;
    data |= internalReadByte(address & ~3 | 0) << 24;
    data |= internalReadByte(address & ~3 | 1) << 16;
    data |= internalReadByte(address & ~3 | 2) <<  8;
    data |= internalReadByte(address & ~3 | 3) <<  0;
    return data;
  }

  return busReadLong(address & ~3);
}

auto SH2::writeByte(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xfff] = data;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    return internalWriteByte(address, data);
  }

  return busWriteByte(address, data);
}

auto SH2::writeWord(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xffe | 0] = data >> 8;
    cache[address & 0xffe | 1] = data >> 0;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    internalWriteByte(address & ~1 | 0, data >> 8);
    internalWriteByte(address & ~1 | 1, data >> 0);
    return;
  }

  return busWriteWord(address & ~1, data);
}

auto SH2::writeLong(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xffc | 0] = data >> 24;
    cache[address & 0xffc | 1] = data >> 16;
    cache[address & 0xffc | 2] = data >>  8;
    cache[address & 0xffc | 3] = data >>  0;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    internalWriteByte(address & ~3 | 0, data >> 24);
    internalWriteByte(address & ~3 | 1, data >> 16);
    internalWriteByte(address & ~3 | 2, data >>  8);
    internalWriteByte(address & ~3 | 3, data >>  0);
    return;
  }

  return busWriteLong(address & ~3, data);
}
