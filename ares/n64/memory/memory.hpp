namespace Memory {

struct Readable {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
    maskDouble = 0;
    maskQuad = 0;
  }

  auto allocate(u32 capacity) -> void {
    size = capacity & ~7;
    u32 mask   = bit::round(size) - 1;
    maskByte   = mask & ~0;
    maskHalf   = mask & ~1;
    maskWord   = mask & ~3;
    maskDouble = mask & ~7;
    maskQuad   = mask & ~15;
    data = new u8[mask + 1];
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(value);
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(fp->readm(4));
    }
  }

  //N64 CPU requires aligned memory accesses
  auto readByte(u32 address) -> u8 { return *(u8*)&data[address & maskByte]; }
  auto readHalf(u32 address) -> u16 { return bswap16(*(u16*)&data[address & maskHalf]); }
  auto readWord(u32 address) -> u32 { return bswap32(*(u32*)&data[address & maskWord]); }
  auto readDouble(u32 address) -> u64 { return bswap64(*(u64*)&data[address & maskDouble]); }
  auto readQuad(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskQuad]); }

  auto writeByte(u32 address, u8 value) -> void {}
  auto writeHalf(u32 address, u16 value) -> void {}
  auto writeWord(u32 address, u32 value) -> void {}
  auto writeDouble(u32 address, u64 value) -> void {}
  auto writeQuad(u32 address, u128 value) -> void {}

  //N64 RSP allows unaligned memory accesses in certain cases
  auto readHalfUnaligned(u32 address) -> u16 { return bswap16(*(u16*)&data[address & maskByte]); }
  auto readWordUnaligned(u32 address) -> u32 { return bswap32(*(u32*)&data[address & maskByte]); }
  auto readDoubleUnaligned(u32 address) -> u64 { return bswap64(*(u64*)&data[address & maskByte]); }
  auto readQuadUnaligned(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskByte]); }

  auto writeHalfUnaligned(u32 address, u16 value) -> void {}
  auto writeWordUnaligned(u32 address, u32 value) -> void {}
  auto writeDoubleUnaligned(u32 address, u64 value) -> void {}
  auto writeQuadUnaligned(u32 address, u128 value) -> void {}

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
  u32 maskDouble = 0;
  u32 maskQuad = 0;
};

struct Writable {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
    maskDouble = 0;
    maskQuad = 0;
  }

  auto allocate(u32 capacity) -> void {
    size = capacity & ~7;
    u32 mask   = bit::round(size) - 1;
    maskByte   = mask & ~0;
    maskHalf   = mask & ~1;
    maskWord   = mask & ~3;
    maskDouble = mask & ~7;
    maskQuad   = mask & ~15;
    data = new u8[mask + 1];
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(value);
      writeWord(address, value);
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(fp->readm(4));
    }
  }

  //N64 CPU requires aligned memory accesses
  auto readByte(u32 address) -> u8 { return *(u8*)&data[address & maskByte]; }
  auto readHalf(u32 address) -> u16 { return bswap16(*(u16*)&data[address & maskHalf]); }
  auto readWord(u32 address) -> u32 { return bswap32(*(u32*)&data[address & maskWord]); }
  auto readDouble(u32 address) -> u64 { return bswap64(*(u64*)&data[address & maskDouble]); }
  auto readQuad(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskQuad]); }

  auto writeByte(u32 address, u8 value) -> void { *(u8*)&data[address & maskByte] = value; }
  auto writeHalf(u32 address, u16 value) -> void { *(u16*)&data[address & maskHalf] = bswap16(value); }
  auto writeWord(u32 address, u32 value) -> void { *(u32*)&data[address & maskWord] = bswap32(value); }
  auto writeDouble(u32 address, u64 value) -> void { *(u64*)&data[address & maskDouble] = bswap64(value); }
  auto writeQuad(u32 address, u128 value) -> void { *(u128*)&data[address & maskQuad] = bswap128(value); }

  //N64 RSP allows unaligned memory accesses in certain cases
  auto readHalfUnaligned(u32 address) -> u16 { return bswap16(*(u16*)&data[address & maskByte]); }
  auto readWordUnaligned(u32 address) -> u32 { return bswap32(*(u32*)&data[address & maskByte]); }
  auto readDoubleUnaligned(u32 address) -> u64 { return bswap64(*(u64*)&data[address & maskByte]); }
  auto readQuadUnaligned(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskByte]); }

  auto writeHalfUnaligned(u32 address, u16 value) -> void { *(u16*)&data[address & maskByte] = bswap16(value); }
  auto writeWordUnaligned(u32 address, u32 value) -> void { *(u32*)&data[address & maskByte] = bswap32(value); }
  auto writeDoubleUnaligned(u32 address, u64 value) -> void { *(u64*)&data[address & maskByte] = bswap64(value); }
  auto writeQuadUnaligned(u32 address, u128 value) -> void { *(u128*)&data[address & maskByte] = bswap128(value); }

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
  u32 maskDouble = 0;
  u32 maskQuad = 0;
};

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

  auto readDouble(u32 address) -> u64 {
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

  auto writeDouble(u32 address, u64 data) -> void {
    ((T*)this)->writeWord(address + 0, data >> 32);
    ((T*)this)->writeWord(address + 4, data >>  0);
  }
};

}

struct Bus {
  //bus.hpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto readDouble(u32 address) -> u64;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;
};

extern Bus bus;
