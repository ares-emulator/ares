//Audio Interface

struct AI : Thread {
  Node::Component node;
  Node::Stream stream;

  //ai.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto sample() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct FIFO {
    uint24 address = 0;
    uint18 length = 0;
    uint16 offset = 0;
    u32 buffer[1 << 16];
  } fifo[2];

  struct IO {
     uint1 dmaEnable = 0;
     uint1 dmaBusy = 0;
    uint14 dacRate = 0;
     uint4 bitRate = 0;

     uint1 fifoPlaying = 0;  //0 or 1
     uint1 fifoWriting = 0;  //0 or 1
     uint2 fifoPending = 0;  //0-2
  } io;

  struct DAC {
    u32 frequency = 44100;
    u32 precision = 16;
    u32 period = 93'750'000 / 44'100;
  } dac;

  static constexpr u32 clockNTSC = 48'681'812;
  static constexpr u32 clockPAL  = 49'656'530;
  static constexpr u32 clockMPAL = 48'628'316;
};

extern AI ai;
