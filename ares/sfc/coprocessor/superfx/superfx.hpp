struct SuperFX : GSU, Thread {
  Node::Object node;
  ReadableMemory rom;
  WritableMemory ram;
  WritableMemory bram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
    } tracer;
  } debugger;

  //superfx.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //bus.cpp
  struct CPUROM : AbstractMemory {
    auto data() -> n8* override;
    auto size() const -> u32 override;
    auto read(n24 address, n8 data) -> n8 override;
    auto write(n24 address, n8 data) -> void override;
  };

  struct CPURAM : AbstractMemory {
    auto data() -> n8* override;
    auto size() const -> u32 override;
    auto read(n24 address, n8 data) -> n8 override;
    auto write(n24 address, n8 data) -> void override;
  };

  struct CPUBRAM : AbstractMemory {
    auto data() -> n8* override;
    auto size() const -> u32 override;
    auto read(n24 address, n8 data) -> n8 override;
    auto write(n24 address, n8 data) -> void override;
  };

  //core.cpp
  auto stop() -> void override;
  auto color(n8 source) -> n8 override;
  auto plot(n8 x, n8 y) -> void override;
  auto rpix(n8 x, n8 y) -> n8 override;

  auto flushPixelCache(PixelCache& cache) -> void;

  //memory.cpp
  auto read(n24 address, n8 data = 0x00) -> n8 override;
  auto write(n24 address, n8 data) -> void override;

  auto readOpcode(n16 address) -> n8;
  auto peekpipe() -> n8;
  auto pipe() -> n8 override;

  auto flushCache() -> void override;
  auto readCache(n16 address) -> n8;
  auto writeCache(n16 address, n8 data) -> void;

  //io.cpp
  auto readIO(n24 address, n8 data) -> n8;
  auto writeIO(n24 address, n8 data) -> void;

  //timing.cpp
  auto step(u32 clocks) -> void override;

  auto syncROMBuffer() -> void override;
  auto readROMBuffer() -> n8 override;
  auto updateROMBuffer() -> void;

  auto syncRAMBuffer() -> void override;
  auto readRAMBuffer(n16 address) -> n8 override;
  auto writeRAMBuffer(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n32 Frequency;

  CPUROM cpurom;
  CPURAM cpuram;
  CPUBRAM cpubram;

private:
  n32 romMask;
  n32 ramMask;
  n32 bramMask;
};

extern SuperFX superfx;
