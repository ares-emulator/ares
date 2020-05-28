//RAMBUS RAM

struct RDRAM : Memory::IO<RDRAM> {
  Node::Component node;
  Memory::Writable ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification io;
    } tracer;

    struct Memory {
      Node::Memory ram;
    } memory;
  } debugger;

  //rdram.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
  } io;
};

extern RDRAM rdram;
