//RAMBUS RAM

struct RDRAM : Memory::IO<RDRAM> {
  Node::Object node;
  Memory::Writable ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //rdram.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    u32 config = 0;
    u32 deviceID = 0;
    u32 delay = 0;
    u32 mode = 0;
    u32 refreshInterval = 0;
    u32 refreshRow = 0;
    u32 rasInterval = 0;
    u32 minInterval = 0;
    u32 addressSelect = 0;
    u32 deviceManufacturer = 0;
  } io;
};

extern RDRAM rdram;
