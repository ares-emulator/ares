template<u32 Origin> auto SH2::readByte(u32 address) -> u32 {
  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) return cache.read<Byte>(address);
    }
    return busReadByte(address & 0x1fff'ffff);
  }

  case Area::Uncached: {
    return busReadByte(address & 0x1fff'ffff);
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.readData<Byte>(address);
    }
    break;
  }

  case Area::IO: {
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(address >= 0xffff'ff00 && address <= 0xffff'ffff)) {
        return exceptions |= AddressErrorCPU, 0;
      }
    }
    return internalReadByte(address);
  }

  }

  return 0;
}

template<u32 Origin> auto SH2::readWord(u32 address) -> u32 {
  if constexpr(Accuracy::AddressErrors) {
    if(unlikely(address & 1)) return exceptions |= AddressErrorCPU, 0;
  }

  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) return cache.read<Word>(address);
    }
    return busReadWord(address & 0x1fff'fffe);
  }

  case Area::Uncached: {
    return busReadWord(address & 0x1fff'fffe);
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.readData<Word>(address);
    }
    break;
  }

  case Area::IO: {
    u16 data = 0;
    data |= internalReadByte(address & ~1 | 0) << 8;
    data |= internalReadByte(address & ~1 | 1) << 0;
    return data;
  }

  }

  return 0;
}

template<u32 Origin> auto SH2::readLong(u32 address) -> u32 {
  if constexpr(Accuracy::AddressErrors) {
    if(unlikely(address & 3)) return exceptions |= AddressErrorCPU, 0;
  }

  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) return cache.read<Long>(address);
    }
    return busReadLong(address & 0x1fff'fffc);
  }

  case Area::Uncached: {
    return busReadLong(address & 0x1fff'fffc);
  }

  case Area::Address: {
    if constexpr(Origin == Bus::Cache) {
      return cache.readAddress(address);
    }
    break;
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.readData<Long>(address);
    }
    break;
  }

  case Area::IO: {
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(address >= 0xffff'fe00 && address <= 0xffff'feff)) {
        return exceptions |= AddressErrorCPU, 0;
      }
    }

    u32 data = 0;
    data |= internalReadByte(address & ~3 | 0) << 24;
    data |= internalReadByte(address & ~3 | 1) << 16;
    data |= internalReadByte(address & ~3 | 2) <<  8;
    data |= internalReadByte(address & ~3 | 3) <<  0;
    return data;
  }

  }

  return 0;
}

template<u32 Origin> auto SH2::writeByte(u32 address, u32 data) -> void {
  if constexpr(Accuracy::Recompiler) {
    recompiler.invalidate(address, 1);
  }

  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) cache.write<Byte>(address, data);
    }
    return busWriteByte(address & 0x1fff'ffff, data);
  }

  case Area::Uncached: {
    cyclesUntilRecompilerExit = 0;
    return busWriteByte(address & 0x1fff'ffff, data);
  }

  case Area::Purge: {
    if constexpr(Origin == Bus::Cache) {
      return cache.purge(address);
    }
    break;
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.writeData<Byte>(address, data);
    }
    break;
  }

  case Area::IO: {
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(address >= 0xffff'ff00 && address <= 0xffff'ffff)) {
        return (void)(exceptions |= AddressErrorCPU);
      }
    }
    return internalWriteByte(address, data);
  }

  }
}

template<u32 Origin> auto SH2::writeWord(u32 address, u32 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(unlikely(address & 1)) return (void)(exceptions |= AddressErrorCPU);
  }

  if constexpr(Accuracy::Recompiler) {
    recompiler.invalidate(address, 2);
  }

  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) cache.write<Word>(address, data);
    }
    return busWriteWord(address & 0x1fff'fffe, data);
  }

  case Area::Uncached: {
    cyclesUntilRecompilerExit = 0;
    return busWriteWord(address & 0x1fff'fffe, data);
  }

  case Area::Purge: {
    if constexpr(Origin == Bus::Cache) {
      return cache.purge(address);
    }
    break;
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.writeData<Word>(address, data);
    }
    break;
  }

  case Area::IO: {
    internalWriteByte(address & ~1 | 0, data >> 8);
    internalWriteByte(address & ~1 | 1, data >> 0);
    return;
  }

  }
}

template<u32 Origin> auto SH2::writeLong(u32 address, u32 data) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(unlikely(address & 3)) return (void)(exceptions |= AddressErrorCPU);
  }

  if constexpr(Accuracy::Recompiler) {
    recompiler.invalidate(address, 4);
  }

  switch(address >> 29) {

  case Area::Cached: {
    if constexpr(Origin == Bus::Cache) {
      if(likely(cache.enable)) cache.write<Long>(address, data);
    }
    return busWriteLong(address & 0x1fff'fffc, data);
  }

  case Area::Uncached: {
    cyclesUntilRecompilerExit = 0;
    return busWriteLong(address & 0x1fff'fffc, data);
  }

  case Area::Purge: {
    if constexpr(Origin == Bus::Cache) {
      return cache.purge(address);
    }
    break;
  }

  case Area::Address: {
    if constexpr(Origin == Bus::Cache) {
      return cache.writeAddress(address, data);
    }
    break;
  }

  case Area::Data: {
    if constexpr(Origin == Bus::Cache) {
      return cache.writeData<Long>(address, data);
    }
    break;
  }

  case Area::IO: {
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(address >= 0xffff'fe00 && address <= 0xffff'feff)) {
        return (void)(exceptions |= AddressErrorCPU);
      }
    }
    internalWriteByte(address & ~3 | 0, data >> 24);
    internalWriteByte(address & ~3 | 1, data >> 16);
    internalWriteByte(address & ~3 | 2, data >>  8);
    internalWriteByte(address & ~3 | 3, data >>  0);
    return;
  }

  }
}
