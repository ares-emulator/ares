//Peripheral Interface

struct PI {
  Node::Component node;
  Memory rom;
  Memory ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory ram;
    } memory;
  } debugger;

  //pi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint1 dmaBusy = 0;
     uint1 ioBusy = 0;
     uint1 error = 0;
     uint1 interrupt = 0;
    uint32 dramAddress = 0;
    uint32 pbusAddress = 0;
    uint32 readLength = 0;
    uint32 writeLength = 0;
  } io;

  struct BSD {
    uint8 latency = 0;
    uint8 pulseWidth = 0;
    uint8 pageSize = 0;
    uint8 releaseDuration = 0;
  } bsd1, bsd2;
};

extern PI pi;
