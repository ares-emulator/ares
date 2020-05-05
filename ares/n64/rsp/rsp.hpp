//Reality Signal Processor

struct RSP : Thread {
  Node::Component node;
  Node::Stream stream;
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
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    u32 memAddress;   //0-11
    u32 memSource;    //0 = dmem, 1 = imem
    u32 dramAddress;  //0-23
    u32 readLength;
    u32 readCount;
    u32 readSkip;
    u32 writeLength;
    u32 writeCount;
    u32 writeSkip;
    bool halt;
    bool interruptOnBreak;
  } io;

  #include "core/core.hpp"
};

extern RSP rsp;
