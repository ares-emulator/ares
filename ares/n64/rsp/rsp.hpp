//Reality Signal Processor

struct RSP : Thread {
  Node::Component node;
  Memory dmem;
  Memory imem;

  //rsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //io-scc.cpp
  auto readSCC(u32 address) -> u32;
  auto writeSCC(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct DMA {
     uint1 memSource = 0;
    uint12 memAddress = 0;
    uint24 dramAddress = 0;
     uint1 busy = 0;
     uint1 full = 0;

    struct Transfer {
      uint12 length = 0;
      uint12 skip = 0;
       uint8 count = 0;
    } read, write;
  } dma;

  struct Status {
     uint1 semaphore = 0;
     uint1 halted = 1;
     uint1 broken = 0;
     uint1 full = 0;
     uint1 singleStep = 0;
     uint1 interruptOnBreak = 0;
     uint1 signal[8] = {};
  } status;

  #include "core/core.hpp"
};

extern RSP rsp;
