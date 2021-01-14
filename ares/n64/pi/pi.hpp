//Peripheral Interface

struct PI : Memory::IO<PI> {
  Node::Object node;
  Memory::Readable rom;
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

  //pi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint1 dmaBusy;
     uint1 ioBusy;
     uint1 error;
     uint1 interrupt;
    uint32 dramAddress;
    uint32 pbusAddress;
    uint32 readLength;
    uint32 writeLength;
  } io;

  struct BSD {
    uint8 latency;
    uint8 pulseWidth;
    uint8 pageSize;
    uint8 releaseDuration;
  } bsd1, bsd2;
};

extern PI pi;
