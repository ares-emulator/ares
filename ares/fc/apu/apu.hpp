struct APU : Thread {
  Node::Component node;
  Node::Stream stream;

  auto rate() const -> uint { return Region::PAL() ? 16 : 12; }

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto tick() -> void;
  auto setIRQ() -> void;

  auto power(bool reset) -> void;

  auto readIO(uint16 address) -> uint8;
  auto writeIO(uint16 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Envelope {
    //envelope.cpp
    auto volume() const -> uint;
    auto clock() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint4 speed;
    uint1 useSpeedAsVolume;
    uint1 loopMode;
    uint1 reloadDecay;
    uint8 decayCounter;
    uint4 decayVolume;
  };

  struct Sweep {
    //sweep.cpp
    auto checkPeriod() -> bool;
    auto clock(uint channel) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint8 shift;
     uint1 decrement;
     uint3 period;
     uint8 counter;
     uint1 enable;
     uint1 reload;
    uint11 pulsePeriod;
  };

  struct Pulse {
    //pulse.cpp
    auto clockLength() -> void;
    auto checkPeriod() -> bool;
    auto clock() -> uint8;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Envelope envelope;
    Sweep sweep;

    uint16 lengthCounter;
    uint16 periodCounter;
     uint2 duty;
     uint3 dutyCounter;
    uint11 period;
  } pulse1, pulse2;

  struct Triangle {
    //triangle.cpp
    auto clockLength() -> void;
    auto clockLinearLength() -> void;
    auto clock() -> uint8;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint16 lengthCounter;
    uint16 periodCounter;
     uint8 linearLength;
     uint1 haltLengthCounter;
    uint11 period;
     uint5 stepCounter;
     uint8 linearLengthCounter;
     uint1 reloadLinear;
  } triangle;

  struct Noise {
    //noise.cpp
    auto clockLength() -> void;
    auto clock() -> uint8;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Envelope envelope;

    uint16 lengthCounter;
    uint16 periodCounter;
     uint4 period;
     uint1 shortMode;
    uint15 lfsr;
  } noise;

  struct DMC {
    //dmc.cpp
    auto start() -> void;
    auto stop() -> void;
    auto clock() -> uint8;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint16 lengthCounter;
    uint16 periodCounter;
    uint16 dmaDelayCounter;
     uint1 irqPending;
     uint4 period;
     uint1 irqEnable;
     uint1 loopMode;
     uint8 dacLatch;
     uint8 addressLatch;
     uint8 lengthLatch;
    uint15 readAddress;
     uint3 bitCounter;
     uint1 dmaBufferValid;
     uint8 dmaBuffer;
     uint1 sampleValid;
     uint8 sample;
  } dmc;

  struct FrameCounter {
    static constexpr uint16_t NtscPeriod = 14915;  //~(21.477MHz / 6 / 240hz)

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint1 irqPending;
    uint2 mode;
    uint2 counter;
    int32 divider;
  } frame;

  //apu.cpp
  auto clockFrameCounter() -> void;
  auto clockFrameCounterDivider() -> void;

  uint5 enabledChannels;

//unserialized:
  int16 pulseDAC[32];
  int16 dmcTriangleNoiseDAC[128][16][16];

  static const  uint8 lengthCounterTable[32];
  static const uint16 dmcPeriodTableNTSC[16];
  static const uint16 dmcPeriodTablePAL[16];
  static const uint16 noisePeriodTableNTSC[16];
  static const uint16 noisePeriodTablePAL[16];
};

extern APU apu;
