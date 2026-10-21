struct Interface {
  struct Cycle {
    u32 address;
    u32 data = 0;
    u32 size;
    u32 byteEnable;
  };

  struct Response {
    u32 data = 0;
  };

  virtual ~Interface() = default;

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

  virtual auto readCycle(const Cycle& cycle) -> Response {
    Response response;
    if(cycle.size == Byte) response.data = readByte(cycle.address);
    if(cycle.size == Half) response.data = readHalf(cycle.address);
    if(cycle.size == Word) response.data = readWord(cycle.address);
    return response;
  }

  virtual auto writeCycle(const Cycle& cycle) -> void {
    if(cycle.size == Byte) return writeByte(cycle.address, cycle.data);
    if(cycle.size == Half) return writeHalf(cycle.address, cycle.data);
    if(cycle.size == Word) return writeWord(cycle.address, cycle.data);
  }

  std::mutex mutex;
};
