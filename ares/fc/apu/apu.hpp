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

    n4 speed;
    n1 useSpeedAsVolume;
    n1 loopMode;
    n1 reloadDecay;
    n8 decayCounter;
    n4 decayVolume;
  };

  struct Sweep {
    //sweep.cpp
    auto checkPeriod() -> bool;
    auto clock(u32 channel) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8  shift;
    n1  decrement;
    n3  period;
    n8  counter = 1;
    n1  enable;
    n1  reload;
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
    n16 periodCounter = 1;
    n2  duty;
    n3  dutyCounter;
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
    n16 periodCounter = 1;
    n8  linearLength;
    n1  haltLengthCounter;
    n11 period;
    n5  stepCounter;
    n8  linearLengthCounter;
    n1  reloadLinear;
  } triangle;

  struct Noise {
    //noise.cpp
    auto clockLength() -> void;
    auto clock() -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Envelope envelope;

    n16 lengthCounter;
    n16 periodCounter = 1;
    n4  period;
    n1  shortMode;
    n15 lfsr = 1;
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
    n1  irqPending;
    n4  period;
    n1  irqEnable;
    n1  loopMode;
    n8  dacLatch;
    n8  addressLatch;
    n8  lengthLatch;
    n15 readAddress;
    n3  bitCounter;
    n1  dmaBufferValid;
    n8  dmaBuffer;
    n1  sampleValid;
    n8  sample;
  } dmc;

  struct FrameCounter {
    static constexpr u16 NtscPeriod = 14915;  //~(21.477MHz / 6 / 240hz)

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  irqPending;
    n2  mode;
    n2  counter;
    i32 divider = 1;
  } frame;

  //apu.cpp
  auto clockFrameCounter() -> void;
  auto clockFrameCounterDivider() -> void;

  n5 enabledChannels;

//unserialized:
  i16 pulseDAC[32];
  i16 dmcTriangleNoiseDAC[128][16][16];

  static const n8  lengthCounterTable[32];
  static const n16 dmcPeriodTableNTSC[16];
  static const n16 dmcPeriodTablePAL[16];
  static const n16 noisePeriodTableNTSC[16];
  static const n16 noisePeriodTablePAL[16];
};

extern APU apu;
