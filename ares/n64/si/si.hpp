//Serial Interface

struct SI : Memory::IO<SI> {
  Node::Component node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification io;
    } tracer;
  } debugger;

  //si.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint24 dramAddress = 0;
    uint32 readAddress = 0;
    uint32 writeAddress = 0;
     uint1 dmaBusy = 0;
     uint1 ioBusy = 0;
     uint1 readPending = 0;
     uint4 pchState = 0;
     uint4 dmaState = 0;
     uint1 dmaError = 0;
     uint1 interrupt = 0;
  } io;

  u64 resetStrobe = 0;  //hack
};

extern SI si;
