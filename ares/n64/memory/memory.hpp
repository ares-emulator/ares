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

  auto allocate(uint32_t capacity) -> void {
    size = capacity & ~7;
    uint32_t mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;  //MIPS does not allow unaligned reads:
    maskWord = mask & ~3;  //this should throw an address exception
    data = new uint8_t[mask + 1];
  }

  auto load(Shared::File fp) -> void {
    for(uint address = 0; address < min(size, fp->size()); address += 4) {
      writeWord(address, fp->readm(4));
    }
  }

  auto readByte(uint32_t address) -> uint8_t {
    return *(uint8_t*)&data[address & maskByte ^ 3];
  }

  auto readHalf(uint32_t address) -> uint16_t {
    return *(uint16_t*)&data[address & maskHalf ^ 2];
  }

  auto readWord(uint32_t address) -> uint32_t {
    return *(uint32_t*)&data[address & maskWord];
  }

  auto readDouble(uint32_t address) -> uint64_t {
    auto value = *(uint64_t*)&data[address & maskWord];
    return value << 32 | value >> 32;
  }

  auto writeByte(uint32_t address, uint8_t value) -> void {
    *(uint8_t*)&data[address & maskByte ^ 3] = value;
  }

  auto writeHalf(uint32_t address, uint16_t value) -> void {
    *(uint16_t*)&data[address & maskHalf ^ 2] = value;
  }

  auto writeWord(uint32_t address, uint32_t value) -> void {
    *(uint32_t*)&data[address & maskWord] = value;
  }

  auto writeDouble(uint32_t address, uint64_t value) -> void {
    *(uint64_t*)&data[address & maskWord] = value << 32 | value >> 32;
  }

private:
  uint8_t* data = nullptr;
  uint32_t size = 0;
  uint32_t maskByte = 0;
  uint32_t maskHalf = 0;
  uint32_t maskWord = 0;
};

//00000000-7fffffff  kuseg
//80000000-9fffffff  kseg0
//a0000000-bfffffff  kseg1
//c0000000-dfffffff  ksseg
//e0000000-ffffffff  kseg3

struct Bus {
  auto readByte(uint32_t address) -> uint8_t;
  auto readHalf(uint32_t address) -> uint16_t;
  auto readWord(uint32_t address) -> uint32_t;
  auto readDouble(uint32_t address) -> uint64_t;
  auto writeByte(uint32_t address, uint8_t data) -> void;
  auto writeHalf(uint32_t address, uint16_t data) -> void;
  auto writeWord(uint32_t address, uint32_t data) -> void;
  auto writeDouble(uint32_t address, uint64_t data) -> void;

  //bus.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
};

extern Bus bus;
extern Memory rdram;
