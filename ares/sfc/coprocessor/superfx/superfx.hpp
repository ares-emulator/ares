struct SuperFX : GSU, Thread {
  Node::Component node;
  ReadableMemory rom;
  WritableMemory ram;
  WritableMemory bram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Instruction instruction;
    } tracer;
  } debugger;

  //superfx.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //bus.cpp
  struct CPUROM : AbstractMemory {
    auto data() -> uint8* override;
    auto size() const -> uint override;
    auto read(uint24 address, uint8 data) -> uint8 override;
    auto write(uint24 address, uint8 data) -> void override;
  };

  struct CPURAM : AbstractMemory {
    auto data() -> uint8* override;
    auto size() const -> uint override;
    auto read(uint24 address, uint8 data) -> uint8 override;
    auto write(uint24 address, uint8 data) -> void override;
  };

  struct CPUBRAM : AbstractMemory {
    auto data() -> uint8* override;
    auto size() const -> uint override;
    auto read(uint24 address, uint8 data) -> uint8 override;
    auto write(uint24 address, uint8 data) -> void override;
  };

  //core.cpp
  auto stop() -> void override;
  auto color(uint8 source) -> uint8 override;
  auto plot(uint8 x, uint8 y) -> void override;
  auto rpix(uint8 x, uint8 y) -> uint8 override;

  auto flushPixelCache(PixelCache& cache) -> void;

  //memory.cpp
  auto read(uint24 address, uint8 data = 0x00) -> uint8 override;
  auto write(uint24 address, uint8 data) -> void override;

  auto readOpcode(uint16 address) -> uint8;
  auto peekpipe() -> uint8;
  auto pipe() -> uint8 override;

  auto flushCache() -> void override;
  auto readCache(uint16 address) -> uint8;
  auto writeCache(uint16 address, uint8 data) -> void;

  //io.cpp
  auto readIO(uint24 address, uint8 data) -> uint8;
  auto writeIO(uint24 address, uint8 data) -> void;

  //timing.cpp
  auto step(uint clocks) -> void override;

  auto syncROMBuffer() -> void override;
  auto readROMBuffer() -> uint8 override;
  auto updateROMBuffer() -> void;

  auto syncRAMBuffer() -> void override;
  auto readRAMBuffer(uint16 address) -> uint8 override;
  auto writeRAMBuffer(uint16 address, uint8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint Frequency;

  CPUROM cpurom;
  CPURAM cpuram;
  CPUBRAM cpubram;

private:
  uint romMask;
  uint ramMask;
  uint bramMask;
};

extern SuperFX superfx;
