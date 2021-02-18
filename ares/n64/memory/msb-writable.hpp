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
    data = memory::alocate<u8, 64_KiB>(mask + 1);
    fill(fillWith);
  }

  auto fill(u32 value = 0) -> void {
    for(u32 address = 0; address < size; address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(value);
    }
  }

  auto load(Shared::File fp) -> void {
    for(u32 address = 0; address < min(size, fp->size()); address += 4) {
      *(u32*)&data[address & maskWord] = bswap32(fp->readm(4L));
    }
  }

  auto save(Shared::File fp) -> void {
    for(u32 address = 0; address < size; address += 4) {
      fp->writem(bswap32(*(u32*)&data[address & maskWord]), 4L);
    }
  }

  //N64 CPU requires aligned memory accesses
  auto readByte(u32 address) -> u8   { return         (*(u8*  )&data[address & maskByte]); }
  auto readHalf(u32 address) -> u16  { return bswap16 (*(u16* )&data[address & maskHalf]); }
  auto readWord(u32 address) -> u32  { return bswap32 (*(u32* )&data[address & maskWord]); }
  auto readDual(u32 address) -> u64  { return bswap64 (*(u64* )&data[address & maskDual]); }
  auto readQuad(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskQuad]); }

  auto writeByte(u32 address, u8   value) -> void { *(u8*  )&data[address & maskByte] =         (value); }
  auto writeHalf(u32 address, u16  value) -> void { *(u16* )&data[address & maskHalf] = bswap16 (value); }
  auto writeWord(u32 address, u32  value) -> void { *(u32* )&data[address & maskWord] = bswap32 (value); }
  auto writeDual(u32 address, u64  value) -> void { *(u64* )&data[address & maskDual] = bswap64 (value); }
  auto writeQuad(u32 address, u128 value) -> void { *(u128*)&data[address & maskQuad] = bswap128(value); }

  //N64 RSP allows unaligned memory accesses in certain cases
  auto readHalfUnaligned(u32 address) -> u16  { return bswap16 (*(u16* )&data[address & maskByte]); }
  auto readWordUnaligned(u32 address) -> u32  { return bswap32 (*(u32* )&data[address & maskByte]); }
  auto readDualUnaligned(u32 address) -> u64  { return bswap64 (*(u64* )&data[address & maskByte]); }
  auto readQuadUnaligned(u32 address) -> u128 { return bswap128(*(u128*)&data[address & maskByte]); }

  auto writeHalfUnaligned(u32 address, u16  value) -> void { *(u16* )&data[address & maskByte] = bswap16 (value); }
  auto writeWordUnaligned(u32 address, u32  value) -> void { *(u32* )&data[address & maskByte] = bswap32 (value); }
  auto writeDualUnaligned(u32 address, u64  value) -> void { *(u64* )&data[address & maskByte] = bswap64 (value); }
  auto writeQuadUnaligned(u32 address, u128 value) -> void { *(u128*)&data[address & maskByte] = bswap128(value); }

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
