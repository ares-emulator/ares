//Reality Signal Processor

struct RSP : Thread, Memory::IO<RSP> {
  Node::Component node;
  Memory::Writable dmem;
  Memory::Writable imem;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification io;
    } tracer;

    struct Memory {
      Node::Memory dmem;
      Node::Memory imem;
    } memory;
  } debugger;

  //rsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

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

  struct Status : Memory::IO<Status> {
    RSP& self;
    Status(RSP& self) : self(self) {}

    //io.cpp
    auto readWord(u32 address) -> u32;
    auto writeWord(u32 address, u32 data) -> void;

    uint1 semaphore = 0;
    uint1 halted = 1;
    uint1 broken = 0;
    uint1 full = 0;
    uint1 singleStep = 0;
    uint1 interruptOnBreak = 0;
    uint1 signal[8] = {};
  } status{*this};

  #include "core/core.hpp"
};

extern RSP rsp;
