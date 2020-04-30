struct Memory {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    mask = 0;
  }

  auto allocate(uint32_t requested) -> void {
    size = requested;
    mask = bit::round(requested) - 1;
    data = new uint8_t[mask + 1];
  }

  auto read(uint32_t address) -> uint8_t {
    return data[address];
  }

  auto write(uint32_t address, uint8_t value) -> void {
    data[address] = value;
  }

  uint8_t* data = nullptr;
  uint32_t size = 0;
  uint32_t mask = 0;
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
  auto readQuad(uint32_t address) -> uint64_t;
  auto writeByte(uint32_t address,  uint8_t data) -> void;
  auto writeHalf(uint32_t address, uint16_t data) -> void;
  auto writeWord(uint32_t address, uint32_t data) -> void;
  auto writeQuad(uint32_t address, uint64_t data) -> void;

  //bus.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
};

extern Bus bus;
extern Memory rdram;
