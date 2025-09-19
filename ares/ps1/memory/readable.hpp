struct Readable : Interface {
  auto reset() -> void {
    delete[] data;
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
  }

  auto allocate(u32 capacity, u32 fillWith = ~0) -> void {
    reset();
    size = capacity & ~3;
    u32 mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;
    maskWord = mask & ~3;
    data = new u8[mask + 1];
    fill(fillWith);
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = value;
    }
  }

  auto load(VFS::File fp) -> void {
    if(!size) allocate(fp->size());
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = fp->readl(4L);
    }
  }

  auto save(VFS::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      fp->writel(*(u32*)&data[address & maskWord], 4L);
    }
  }

  auto readByte(u32 address) -> u32 { return  *(u8*)&data[address & maskByte]; }
  auto readHalf(u32 address) -> u32 { return *(u16*)&data[address & maskHalf]; }
  auto readWord(u32 address) -> u32 { return *(u32*)&data[address & maskWord]; }

  auto writeByte(u32 address, u32 data) -> void {}
  auto writeHalf(u32 address, u32 data) -> void {}
  auto writeWord(u32 address, u32 data) -> void {}

  auto serialize(serializer& s) -> void {
  //s(std::span<u8>{data, size});
  }

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
};
