struct APU : Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 cycle, n16 address, n8 data) -> n8;
  auto writeIO(u32 cycle, n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //square1.cpp
  struct Square1 {
    auto dacEnable() const -> bool;

    auto run() -> void;
    auto sweep(bool update) -> void;
    auto clockLength() -> void;
    auto clockSweep() -> void;
    auto clockEnvelope() -> void;
    auto trigger() -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    n3 sweepFrequency;
    bool sweepDirection;
    n3 sweepShift;
    bool sweepNegate;
    n2 duty;
    u32 length;
    n4 envelopeVolume;
    bool envelopeDirection;
    n3 envelopeFrequency;
    n11 frequency;
    bool counter;

    i16 output;
    bool dutyOutput;
    n3 phase;
    u32 period;
    n3 envelopePeriod;
    n3 sweepPeriod;
    s32 frequencyShadow;
    bool sweepEnable;
    n4 volume;
  } square1;

  //square2.cpp
  struct Square2 {
    auto dacEnable() const -> bool;

    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;
    auto trigger() -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    n2 duty;
    u32 length;
    n4 envelopeVolume;
    bool envelopeDirection;
    n3 envelopeFrequency;
    n11 frequency;
    bool counter;

    i16 output;
    bool dutyOutput;
    n3 phase;
    u32 period;
    n3 envelopePeriod;
    n4 volume;
  } square2;

  struct Wave {
    auto getPattern(n5 offset) const -> n4;

    auto run() -> void;
    auto clockLength() -> void;
    auto trigger() -> void;
    auto readRAM(n4 address, n8 data) -> n8;
    auto writeRAM(n4 address, n8 data) -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    bool dacEnable;
    n2 volume;
    n11 frequency;
    bool counter;
    n8 pattern[16];

    i16 output;
    u32 length;
    u32 period;
    n5 patternOffset;
    n4 patternSample;
    u32 patternHold;
  } wave;

  struct Noise {
    auto dacEnable() const -> bool;
    auto getPeriod() const -> u32;

    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;
    auto trigger() -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    n4 envelopeVolume;
    bool envelopeDirection;
    n3 envelopeFrequency;
    n4 frequency;
    bool narrow;
    n3 divisor;
    bool counter;

    i16 output;
    u32 length;
    n3 envelopePeriod;
    n4 volume;
    u32 period;
    n15 lfsr;
  } noise;

  struct Sequencer {
    auto run() -> void;
    auto power() -> void;

    auto serialize(serializer&) -> void;

    bool leftEnable;
    n3 leftVolume;
    bool rightEnable;
    n3 rightVolume;

    struct Channel {
      bool leftEnable;
      bool rightEnable;
    } square1, square2, wave, noise;

    bool enable;

    i16 center;
    i16 left;
    i16 right;
  } sequencer;

  n3 phase;   //high 3-bits of clock counter
  n12 cycle;  //low 12-bits of clock counter
};

extern APU apu;
