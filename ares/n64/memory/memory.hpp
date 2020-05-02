//the N64 is configured for big-endian mode.
//given the most common memory accesses are words,
//optimize accesses by special-casing bytes anf halves instead.
struct Memory {
  auto bytes()   const -> uint { return size >> 0; }
  auto halves()  const -> uint { return size >> 1; }
  auto words()   const -> uint { return size >> 2; }
  auto doubles() const -> uint { return size >> 3; }

  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
  }

  auto allocate(u32 capacity) -> void {
    size = capacity & ~7;
    u32 mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;  //MIPS does not allow unaligned reads:
    maskWord = mask & ~3;  //this should throw an address exception
    data = new u8[mask + 1];
  }

  auto load(Shared::File fp) -> void {
    for(uint address = 0; address < min(size, fp->size()); address += 4) {
      writeWord(address, fp->readm(4));
    }
  }

  auto readByte(u32 address) -> u8 {
    return *(u8*)&data[address & maskByte ^ 3];
  }

  auto readHalf(u32 address) -> u16 {
    return *(u16*)&data[address & maskHalf ^ 2];
  }

  auto readWord(u32 address) -> u32 {
    return *(u32*)&data[address & maskWord];
  }

  auto readDouble(u32 address) -> u64 {
    auto value = *(u64*)&data[address & maskWord];
    return value << 32 | value >> 32;
  }

  auto writeByte(u32 address, u8 value) -> void {
    *(u8*)&data[address & maskByte ^ 3] = value;
  }

  auto writeHalf(u32 address, u16 value) -> void {
    *(u16*)&data[address & maskHalf ^ 2] = value;
  }

  auto writeWord(u32 address, u32 value) -> void {
    *(u32*)&data[address & maskWord] = value;
  }

  auto writeDouble(u32 address, u64 value) -> void {
    *(u64*)&data[address & maskWord] = value << 32 | value >> 32;
  }

private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
};

//00000000-7fffffff  kuseg
//80000000-9fffffff  kseg0
//a0000000-bfffffff  kseg1
//c0000000-dfffffff  ksseg
//e0000000-ffffffff  kseg3

struct Bus {
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto readDouble(u32 address) -> u64;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;

  //bus.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
};

extern Bus bus;
extern Memory rdram;
