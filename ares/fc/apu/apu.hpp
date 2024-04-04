struct APU {
  Node::Object node;
  Node::Audio::Stream stream;

  auto rate() const -> u32 { return Region::PAL() ? 16 : 12; }

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
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
    n5  stepCounter = 16;
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
    enum mode: u8 { Freq60Hz = 0, Freq48Hz = 1 };

    auto rate() const -> u32 { return Region::PAL() ? 16 : 12; }
    auto getPeriod() const -> u16 {
      return Region::PAL() ? periodPAL[mode][step] : periodNTSC[mode][step];
    }

    auto main() -> void;
    auto power(bool reset) -> void;
    auto write(n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  irqInhibit;
    n1  mode;
    n1  newMode;

    n1  irqPending;
    n3  step;
    n16 counter;

    bool odd;
    bool delay;
    n3   delayCounter;

    constexpr static u16 periodNTSC[2][6] = {
      { 7457, 7456, 7458, 7457,    1, 1, },
      { 7457, 7456, 7458, 7458, 7452, 1, },
    };
    constexpr static u16 periodPAL[2][6] = {
      { 8313, 8314, 8312, 8313,    1, 1, },
      { 8313, 8314, 8312, 8320, 8312, 1,},
    };
  };

  //apu.cpp
  auto clockQuarterFrame() -> void;
  auto clockHalfFrame() -> void;

  FrameCounter frame;
  n5 enabledChannels;

//unserialized:
  u16 pulseDAC[32];
  u16 dmcTriangleNoiseDAC[128][16][16];

  static const n8  lengthCounterTable[32];
  static const n16 dmcPeriodTableNTSC[16];
  static const n16 dmcPeriodTablePAL[16];
  static const n16 noisePeriodTableNTSC[16];
  static const n16 noisePeriodTablePAL[16];
};

extern APU apu;
