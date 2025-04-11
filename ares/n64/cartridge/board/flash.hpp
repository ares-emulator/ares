struct Flash : Memory::Writable16 {
  template<u32 Size>
  auto read(u32 address) -> u64 {
    if constexpr(Size == Byte) return readByte(address);
    if constexpr(Size == Half) return readHalf(address);
    if constexpr(Size == Word) return readWord(address);
    if constexpr(Size == Dual) return readDual(address);
    unreachable;
  }

  template<u32 Size>
  auto write(u32 address, u64 data) -> void {
    if constexpr(Size == Byte) return writeByte(address, data);
    if constexpr(Size == Half) return writeHalf(address, data);
    if constexpr(Size == Word) return writeWord(address, data);
    if constexpr(Size == Dual) return writeDual(address, data);
  }

  //flash.cpp
  auto readByte(u32 adddres) -> u64;
  auto readHalf(u32 address) -> u64;
  auto readWord(u32 address) -> u64;
  auto readDual(u32 address) -> u64;
  auto writeByte(u32 address, u64 data) -> void;
  auto writeHalf(u32 address, u64 data) -> void;
  auto writeWord(u32 address, u64 data) -> void;
  auto writeDual(u32 address, u64 data) -> void;

  enum class Mode : u32 { Idle, Erase, Write, Read, Status };
  Mode mode = Mode::Idle;
  u64  status = 0;
  u32  source = 0;
  u32  offset = 0;
} flash;
