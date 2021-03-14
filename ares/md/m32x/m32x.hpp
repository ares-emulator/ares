//Mega 32X

struct M32X {
  Node::Object node;
  Memory::Readable<n16> rom;
  Memory::Readable<n16> vectors;
  Memory::Writable<n16> dram;
  Memory::Writable<n16> sdram;
  Memory::Writable<n16> palette;

  struct SHM : SH2, Thread {
    Node::Object node;
    Memory::Readable<n16> bootROM;

    struct Debugger {
      //debugger.cpp
      auto load(Node::Object) -> void;
      auto instruction() -> void;
      auto interrupt(string_view) -> void;

      struct Tracer {
        Node::Debugger::Tracer::Instruction instruction;
        Node::Debugger::Tracer::Notification interrupt;
      } tracer;
    } debugger;

    //shm.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto main() -> void;
    auto step(u32 clocks) -> void override;
    auto exception() -> bool override;
    auto readByte(u32 address) -> u32 override;
    auto readWord(u32 address) -> u32 override;
    auto readLong(u32 address) -> u32 override;
    auto writeByte(u32 address, u32 data) -> void override;
    auto writeWord(u32 address, u32 data) -> void override;
    auto writeLong(u32 address, u32 data) -> void override;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  } shm;

  struct SHS : SH2, Thread {
    Node::Object node;
    Memory::Readable<n16> bootROM;

    struct Debugger {
      //debugger.cpp
      auto load(Node::Object) -> void;
      auto instruction() -> void;
      auto interrupt(string_view) -> void;

      struct Tracer {
        Node::Debugger::Tracer::Instruction instruction;
        Node::Debugger::Tracer::Notification interrupt;
      } tracer;
    } debugger;

    //shs.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto main() -> void;
    auto step(u32 clocks) -> void override;
    auto exception() -> bool override;
    auto readByte(u32 address) -> u32 override;
    auto readWord(u32 address) -> u32 override;
    auto readLong(u32 address) -> u32 override;
    auto writeByte(u32 address, u32 data) -> void override;
    auto writeWord(u32 address, u32 data) -> void override;
    auto writeLong(u32 address, u32 data) -> void override;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  } shs;

  //m32x.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto save() -> void;

  auto main() -> void;
  auto power(bool reset) -> void;

  //bus-internal.cpp
  auto readInternal(n1 upper, n1 lower, n29 address, n16 data = 0) -> n16;
  auto writeInternal(n1 upper, n1 lower, n29 address, n16 data) -> void;

  //bus-external.cpp
  auto readExternal(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeExternal(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //io-internal.cpp
  auto readInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> n16;
  auto writeInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> void;

  //io-external.cpp
  auto readExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    //$a15000
    n1 adapterEnable;
    n1 adapterReset;
    n1 resetEnable = 1;
    n1 framebufferAccess;

    //$a15002
    n1 intm;
    n1 ints;

    //$a15004
    n1 bank0;
    n1 bank1;

    //$a1511a
    n1 cartridgeMode;
  } io;

  struct DREQ {
    n24 source;
    n24 target;
    n16 length;
  } dreq;

  n16 communication[8];
};

extern M32X m32x;
