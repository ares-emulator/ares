struct APU : Thread {
  Node::Component node;
  Node::Stream stream;

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(uint cycle, uint16 address, uint8 data) -> uint8;
  auto writeIO(uint cycle, uint16 address, uint8 data) -> void;

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

    uint3 sweepFrequency;
    bool sweepDirection;
    uint3 sweepShift;
    bool sweepNegate;
    uint2 duty;
    uint length;
    uint4 envelopeVolume;
    bool envelopeDirection;
    uint3 envelopeFrequency;
    uint11 frequency;
    bool counter;

    int16 output;
    bool dutyOutput;
    uint3 phase;
    uint period;
    uint3 envelopePeriod;
    uint3 sweepPeriod;
    int frequencyShadow;
    bool sweepEnable;
    uint4 volume;
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

    uint2 duty;
    uint length;
    uint4 envelopeVolume;
    bool envelopeDirection;
    uint3 envelopeFrequency;
    uint11 frequency;
    bool counter;

    int16 output;
    bool dutyOutput;
    uint3 phase;
    uint period;
    uint3 envelopePeriod;
    uint4 volume;
  } square2;

  struct Wave {
    auto getPattern(uint5 offset) const -> uint4;

    auto run() -> void;
    auto clockLength() -> void;
    auto trigger() -> void;
    auto readRAM(uint4 address, uint8 data) -> uint8;
    auto writeRAM(uint4 address, uint8 data) -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    bool dacEnable;
    uint2 volume;
    uint11 frequency;
    bool counter;
    uint8 pattern[16];

    int16 output;
    uint length;
    uint period;
    uint5 patternOffset;
    uint4 patternSample;
    uint patternHold;
  } wave;

  struct Noise {
    auto dacEnable() const -> bool;
    auto getPeriod() const -> uint;

    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;
    auto trigger() -> void;
    auto power(bool initializeLength = true) -> void;

    auto serialize(serializer&) -> void;

    bool enable;

    uint4 envelopeVolume;
    bool envelopeDirection;
    uint3 envelopeFrequency;
    uint4 frequency;
    bool narrow;
    uint3 divisor;
    bool counter;

    int16 output;
    uint length;
    uint3 envelopePeriod;
    uint4 volume;
    uint period;
    uint15 lfsr;
  } noise;

  struct Sequencer {
    auto run() -> void;
    auto power() -> void;

    auto serialize(serializer&) -> void;

    bool leftEnable;
    uint3 leftVolume;
    bool rightEnable;
    uint3 rightVolume;

    struct Channel {
      bool leftEnable;
      bool rightEnable;
    } square1, square2, wave, noise;

    bool enable;

    int16 center;
    int16 left;
    int16 right;
  } sequencer;

  uint3 phase;   //high 3-bits of clock counter
  uint12 cycle;  //low 12-bits of clock counter
};

extern APU apu;
