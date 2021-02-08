template<typename T>
struct IO {
  auto readByte(u32 address) -> u8 {
    auto data = ((T*)this)->readWord(address);
    switch(address & 3) {
    case 0: return data >> 24;
    case 1: return data >> 16;
    case 2: return data >>  8;
    case 3: return data >>  0;
    } unreachable;
  }

  auto readHalf(u32 address) -> u16 {
    auto data = ((T*)this)->readWord(address);
    switch(address & 2) {
    case 0: return data >> 16;
    case 2: return data >>  0;
    } unreachable;
  }

  auto readDual(u32 address) -> u64 {
    u64 data = ((T*)this)->readWord(address);
    return data << 32 | ((T*)this)->readWord(address + 4);
  }

  auto writeByte(u32 address, u8 data) -> void {
    switch(address & 3) {
    case 0: return ((T*)this)->writeWord(address, data << 24);
    case 1: return ((T*)this)->writeWord(address, data << 16);
    case 2: return ((T*)this)->writeWord(address, data <<  8);
    case 3: return ((T*)this)->writeWord(address, data <<  0);
    } unreachable;
  }

  auto writeHalf(u32 address, u16 data) -> void {
    switch(address & 2) {
    case 0: return ((T*)this)->writeWord(address, data << 16);
    case 2: return ((T*)this)->writeWord(address, data <<  0);
    } unreachable;
  }

  auto writeDual(u32 address, u64 data) -> void {
    ((T*)this)->writeWord(address + 0, data >> 32);
    ((T*)this)->writeWord(address + 4, data >>  0);
  }
};
