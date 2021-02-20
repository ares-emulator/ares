//RDRAM Interface

struct RI : Memory::IO<RI> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

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
    u32 mode = 0x0e;
    u32 config = 0x40;
    u32 currentLoad = 0x00;
    u32 select = 0x14;
    u32 refresh = 0x0006'3634;
    u32 latency = 0;
    u32 readError = 0;
    u32 writeError = 0;
  } io;
};

extern RI ri;
