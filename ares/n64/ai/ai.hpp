//Audio Interface

struct AI : Thread, Memory::IO<AI> {
  Node::Component node;
  Node::Stream stream;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification io;
    } tracer;
  } debugger;

  //ai.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto sample() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    uint24 address = 0;
  } fifo[2];

  struct IO {
     uint1 dmaEnable = 0;
    uint24 dmaAddress[2] = {};
    uint18 dmaLength[2] = {};
     uint2 dmaCount = 0;
    uint14 dacRate = 0;
     uint4 bitRate = 0;
  } io;

  struct DAC {
    u32 frequency = 44100;
    u32 precision = 16;
    u32 period = 93'750'000 / 44'100;
  } dac;
};

extern AI ai;
