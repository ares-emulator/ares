struct Writable {
  explicit operator bool() const {
    return size > 0;
  }

  auto reset() -> void {
    memory::free<u8, 64_KiB>(data);
    data = nullptr;
    size = 0;
    maskByte = 0;
    maskHalf = 0;
    maskWord = 0;
    maskDual = 0;
    maskQuad = 0;
  }

  auto allocate(u32 capacity, u32 fillWith = ~0) -> void {
    reset();
    size = capacity & ~15;
    u32 mask = bit::round(size) - 1;
    maskByte = mask & ~0;
    maskHalf = mask & ~1;
    maskWord = mask & ~3;
    maskDual = mask & ~7;
    maskQuad = mask & ~15;
    data = memory::allocate<u8, 64_KiB>(mask + 1);
    fill(fillWith);
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = value;
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = fp->readm(4L);
    }
  }

  auto save(Shared::File fp) -> void {
    for(u32 address = 0; address < size; address += 4) {
      fp->writem(*(u32*)&data[address & maskWord], 4L);
    }
  }

  //N64 CPU requires aligned memory accesses
  auto readByte(u32 address) -> u8  { return *(u8* )&data[address & maskByte ^ 3]; }
  auto readHalf(u32 address) -> u16 { return *(u16*)&data[address & maskHalf ^ 2]; }
  auto readWord(u32 address) -> u32 { return *(u32*)&data[address & maskWord ^ 0]; }
  auto readDual(u32 address) -> u64 {
    u64 upper = readWord(address + 0);
    u64 lower = readWord(address + 4);
    return upper << 32 | lower << 0;
  }
  auto readQuad(u32 address) -> u128 {
    u128 upper = readDual(address + 0);
    u128 lower = readDual(address + 8);
    return upper << 64 | lower << 0;
  }

  auto writeByte(u32 address, u8  value) -> void { *(u8* )&data[address & maskByte ^ 3] = value; }
  auto writeHalf(u32 address, u16 value) -> void { *(u16*)&data[address & maskHalf ^ 2] = value; }
  auto writeWord(u32 address, u32 value) -> void { *(u32*)&data[address & maskWord ^ 0] = value; }
  auto writeDual(u32 address, u64 value) -> void {
    writeWord(address + 0, value >> 32);
    writeWord(address + 4, value >>  0);
  }
  auto writeQuad(u32 address, u128 value) -> void {
    writeDual(address + 0, value >> 64);
    writeDual(address + 8, value >>  0);
  }

  //N64 RSP allows unaligned memory accesses in certain cases
  auto readHalfUnaligned(u32 address) -> u16 {
    u16 upper = readByte(address + 0);
    u16 lower = readByte(address + 1);
    return upper << 8 | lower << 0;
  }
  auto readWordUnaligned(u32 address) -> u32 {
    u32 upper = readHalfUnaligned(address + 0);
    u32 lower = readHalfUnaligned(address + 2);
    return upper << 16 | lower << 0;
  }
  auto readDualUnaligned(u32 address) -> u64 {
    u64 upper = readWordUnaligned(address + 0);
    u64 lower = readWordUnaligned(address + 4);
    return upper << 32 | lower << 0;
  }
  auto readQuadUnaligned(u32 address) -> u128 {
    u128 upper = readDualUnaligned(address + 0);
    u128 lower = readDualUnaligned(address + 8);
    return upper << 64 | lower << 0;
  }

  auto writeHalfUnaligned(u32 address, u16 value) -> void {
    writeByte(address + 0, value >> 8);
    writeByte(address + 1, value >> 0);
  }
  auto writeWordUnaligned(u32 address, u32 value) -> void {
    writeHalfUnaligned(address + 0, value >> 16);
    writeHalfUnaligned(address + 2, value >>  0);
  }
  auto writeDualUnaligned(u32 address, u64 value) -> void {
    writeWordUnaligned(address + 0, value >> 32);
    writeWordUnaligned(address + 4, value >>  0);
  }
  auto writeQuadUnaligned(u32 address, u128 value) -> void {
    writeDualUnaligned(address + 0, value >> 64);
    writeDualUnaligned(address + 8, value >>  0);
  }

  auto serialize(serializer& s) -> void {
    s(array_span<u8>{data, size});
  }

//private:
  u8* data = nullptr;
  u32 size = 0;
  u32 maskByte = 0;
  u32 maskHalf = 0;
  u32 maskWord = 0;
  u32 maskDual = 0;
  u32 maskQuad = 0;
};
