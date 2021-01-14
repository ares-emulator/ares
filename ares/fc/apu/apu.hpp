struct APU : Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  auto rate() const -> u32 { return Region::PAL() ? 16 : 12; }

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto tick() -> void;
  auto setIRQ() -> void;

  auto power(bool reset) -> void;

  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Envelope {
    //envelope.cpp
    auto volume() const -> u32;
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n04 speed;
    n01 useSpeedAsVolume;
    n01 loopMode;
    n01 reloadDecay;
    n08 decayCounter;
    n04 decayVolume;
  };

  struct Sweep {
    //sweep.cpp
    auto checkPeriod() -> bool;
    auto clock(u32 channel) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n08 shift;
    n01 decrement;
    n03 period;
    n08 counter;
    n01 enable;
    n01 reload;
    n11 pulsePeriod;
  };

  struct Pulse {
    //pulse.cpp
    auto clockLength() -> void;
    auto checkPeriod() -> bool;
    auto clock() -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Envelope envelope;
    Sweep sweep;

    n16 lengthCounter;
    n16 periodCounter;
    n02 duty;
    n03 dutyCounter;
    n11 period;
  } pulse1, pulse2;

  struct Triangle {
    //triangle.cpp
    auto clockLength() -> void;
    auto clockLinearLength() -> void;
    auto clock() -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n16 lengthCounter;
    n16 periodCounter;
    n08 linearLength;
    n01 haltLengthCounter;
    n11 period;
    n05 stepCounter;
    n08 linearLengthCounter;
    n01 reloadLinear;
  } triangle;

  struct Noise {
    //noise.cpp
    auto clockLength() -> void;
    auto clock() -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Envelope envelope;

    n16 lengthCounter;
    n16 periodCounter;
    n04 period;
    n01 shortMode;
    n15 lfsr;
  } noise;

  struct DMC {
    //dmc.cpp
    auto start() -> void;
    auto stop() -> void;
    auto clock() -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n16 lengthCounter;
    n16 periodCounter;
    n16 dmaDelayCounter;
    n01 irqPending;
    n04 period;
    n01 irqEnable;
    n01 loopMode;
    n08 dacLatch;
    n08 addressLatch;
    n08 lengthLatch;
    n15 readAddress;
    n03 bitCounter;
    n01 dmaBufferValid;
    n08 dmaBuffer;
    n01 sampleValid;
    n08 sample;
  } dmc;

  struct FrameCounter {
    static constexpr u16 NtscPeriod = 14915;  //~(21.477MHz / 6 / 240hz)

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n01 irqPending;
    n02 mode;
    n02 counter;
    i32 divider;
  } frame;

  //apu.cpp
  auto clockFrameCounter() -> void;
  auto clockFrameCounterDivider() -> void;

  n05 enabledChannels;

//unserialized:
  i16 pulseDAC[32];
  i16 dmcTriangleNoiseDAC[128][16][16];

  static const n08 lengthCounterTable[32];
  static const n16 dmcPeriodTableNTSC[16];
  static const n16 dmcPeriodTablePAL[16];
  static const n16 noisePeriodTableNTSC[16];
  static const n16 noisePeriodTablePAL[16];
};

extern APU apu;
