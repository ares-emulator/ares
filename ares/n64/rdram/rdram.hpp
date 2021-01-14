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
};

extern RDRAM rdram;
