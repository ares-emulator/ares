struct Interface {
  virtual ~Interface() = default;

  template<u32 Size> auto wait() const -> u32 {
    if constexpr(Size == Byte) return waitStates.byte;
    if constexpr(Size == Half) return waitStates.half;
    if constexpr(Size == Word) return waitStates.word;
  }

  template<u32 Size> auto read(u32 address) -> u32 {
    if constexpr(Size == Byte) return readByte(address);
    if constexpr(Size == Half) return readHalf(address);
    if constexpr(Size == Word) return readWord(address);
    unreachable;
  }

  template<u32 Size> auto write(u32 address, u32 data) -> void {
    if constexpr(Size == Byte) return writeByte(address, data);
    if constexpr(Size == Half) return writeHalf(address, data);
    if constexpr(Size == Word) return writeWord(address, data);
  }

  virtual auto readByte(u32 address) -> u32 = 0;
  virtual auto readHalf(u32 address) -> u32 = 0;
  virtual auto readWord(u32 address) -> u32 = 0;
  virtual auto writeByte(u32 address, u32 data) -> void = 0;
  virtual auto writeHalf(u32 address, u32 data) -> void = 0;
  virtual auto writeWord(u32 address, u32 data) -> void = 0;

  auto setWaitStates(u32 byte, u32 half, u32 word) -> void {
    waitStates.byte = byte;
    waitStates.half = half;
    waitStates.word = word;
  }

  //additional clock cycles required to access memory
  struct WaitStates {
    u32 byte = 0;
    u32 half = 0;
    u32 word = 0;
  } waitStates;

  std::mutex mutex;
};
