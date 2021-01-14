struct Interface {
  virtual ~Interface() = default;

  template<uint Size> auto wait() const -> uint {
    if constexpr(Size == Byte) return waitStates.byte;
    if constexpr(Size == Half) return waitStates.half;
    if constexpr(Size == Word) return waitStates.word;
  }

  template<uint Size> auto read(u32 address) -> u32 {
    if constexpr(Size == Byte) return readByte(address);
    if constexpr(Size == Half) return readHalf(address);
    if constexpr(Size == Word) return readWord(address);
    unreachable;
  }

  template<uint Size> auto write(u32 address, u32 data) -> void {
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

  auto setWaitStates(uint byte, uint half, uint word) -> void {
    waitStates.byte = byte;
    waitStates.half = half;
    waitStates.word = word;
  }

  //additional clock cycles required to access memory
  struct WaitStates {
    uint byte = 0;
    uint half = 0;
    uint word = 0;
  } waitStates;

  std::mutex mutex;
};
