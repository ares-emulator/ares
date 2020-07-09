namespace Memory {

struct Readable {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
  }

  auto allocate(u32 capacity) -> void {
    size = capacity & ~3;
    u32 mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;
    maskWord = mask & ~3;
    data = new u8[mask + 1];
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = value;
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = fp->readl(4);
    }
  }

  auto readByte(u32 address) const -> u8 { return *(u8*)&data[address & maskByte]; }
  auto readHalf(u32 address) const -> u16 { return *(u16*)&data[address & maskHalf]; }
  auto readWord(u32 address) const -> u32 { return *(u32*)&data[address & maskWord]; }

  auto writeByte(u32 address, u8 data) -> void {}
  auto writeHalf(u32 address, u16 data) -> void {}
  auto writeWord(u32 address, u32 data) -> void {}

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
};

struct Writable {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
  }

  auto allocate(u32 capacity) -> void {
    size = capacity & ~3;
    u32 mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;
    maskWord = mask & ~3;
    data = new u8[mask + 1];
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = value;
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = fp->readl(4);
    }
  }

  auto readByte(u32 address) const -> u8 { return *(u8*)&data[address & maskByte]; }
  auto readHalf(u32 address) const -> u16 { return *(u16*)&data[address & maskHalf]; }
  auto readWord(u32 address) const -> u32 { return *(u32*)&data[address & maskWord]; }

  auto writeByte(u32 address, u8 value) -> void { *(u8*)&data[address & maskByte] = value; }
  auto writeHalf(u32 address, u16 value) -> void { *(u16*)&data[address & maskHalf] = value; }
  auto writeWord(u32 address, u32 value) -> void { *(u32*)&data[address & maskWord] = value; }

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
};

}

struct Bus {
  //bus.hpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
};

extern Bus bus;
extern Memory::Readable bios;
