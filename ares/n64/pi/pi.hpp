//Peripheral Interface

struct PI : Memory::IO<PI> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //pi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //dma.cpp
  auto dmaRead() -> void;
  auto dmaWrite() -> void;
  auto dmaFinished() -> void;

  //io.cpp
  auto ioRead(u32 address) -> u32;
  auto ioWrite(u32 address, u32 data) -> void;

  //bus.hpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeFinished() -> void;
  auto writeForceFinish() -> void;
  template <u32 Size>
  auto busRead(u32 address) -> u32;
  template <u32 Size>
  auto busWrite(u32 address, u32 data) -> void;
  
  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1  dmaBusy;
    n1  ioBusy;
    n1  error;
    n1  interrupt;
    n32 dramAddress;
    n32 pbusAddress;
    n32 readLength;
    n32 writeLength;
    n32 busLatch;
  } io;

  struct BSD {
    n8 latency;
    n8 pulseWidth;
    n8 pageSize;
    n8 releaseDuration;
  } bsd1, bsd2;
};

extern PI pi;
