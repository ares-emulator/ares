//Audio Interface

struct AI : Thread, Memory::IO<AI> {
  Node::Object node;
  Node::Audio::Stream stream;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //ai.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto sample() -> void;
  auto step(uint clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    uint24 address;
  } fifo[2];

  struct IO {
     uint1 dmaEnable;
    uint24 dmaAddress[2];
    uint18 dmaLength[2];
     uint2 dmaCount;
    uint14 dacRate;
     uint4 bitRate;
  } io;

  struct DAC {
    u32 frequency;
    u32 precision;
    u32 period;
  } dac;
};

extern AI ai;
