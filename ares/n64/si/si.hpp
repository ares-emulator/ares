//Serial Interface

struct SI : Memory::IO<SI> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //si.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint24 dramAddress;
    uint32 readAddress;
    uint32 writeAddress;
     uint1 dmaBusy;
     uint1 ioBusy;
     uint1 readPending;
     uint4 pchState;
     uint4 dmaState;
     uint1 dmaError;
     uint1 interrupt;
  } io;

  u64 resetStrobe;  //hack
};

extern SI si;
