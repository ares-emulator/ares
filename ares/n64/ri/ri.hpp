//RDRAM Interface

struct RI : Memory::IO<RI> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //ri.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n32 mode = 0x0e;
    n32 config = 0x40;
    n32 currentLoad = 0x00;
    n32 select = 0x14;
    n32 refresh = 0x0006'3634;
    n32 latency = 0;
    n32 readError = 0;
    n32 writeError = 0;
  } io;
};

extern RI ri;
