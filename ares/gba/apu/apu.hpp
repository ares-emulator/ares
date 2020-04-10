struct APU : Thread, IO {
  Node::Component node;
  Node::Stream stream;

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto readIO(uint32 addr) -> uint8;
  auto writeIO(uint32 addr, uint8 byte) -> void;
  auto power() -> void;

  //sequencer.cpp
  auto sequence() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint clock;

  struct Bias {
    uint10 level = 0x200;
     uint2 amplitude = 0;
  } bias;

  struct Sweep {
    uint3 shift;
    uint1 direction;
    uint3 frequency;

    uint1 enable;
    uint1 negate;
    uint3 period;
  };

  struct Envelope {
    auto dacEnable() const -> bool { return volume || direction; }

    uint3 frequency;
    uint1 direction;
    uint4 volume;

    uint3 period;
  };

  struct Square {
    //square.cpp
    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;

    Envelope envelope;

     uint1 enable;
     uint6 length;
     uint2 duty;
    uint11 frequency;
     uint1 counter;
     uint1 initialize;

     int32 shadowfrequency;
     uint1 signal;
     uint4 output;
    uint32 period;
     uint3 phase;
     uint4 volume;
  };

  struct Square1 : Square {
    //square1.cpp
    auto runSweep(bool update) -> void;
    auto clockSweep() -> void;
    auto read(uint addr) const -> uint8;
    auto write(uint addr, uint8 byte) -> void;
    auto power() -> void;

    Sweep sweep;
  } square1;

  struct Square2 : Square {
    //square2.cpp
    auto read(uint addr) const -> uint8;
    auto write(uint addr, uint8 byte) -> void;
    auto power() -> void;
  } square2;

  struct Wave {
    //wave.cpp
    auto run() -> void;
    auto clockLength() -> void;
    auto read(uint addr) const -> uint8;
    auto write(uint addr, uint8 byte) -> void;
    auto readRAM(uint addr) const -> uint8;
    auto writeRAM(uint addr, uint8 byte) -> void;
    auto power() -> void;

     uint1 mode;
     uint1 bank;
     uint1 dacenable;
     uint8 length;
     uint3 volume;
    uint11 frequency;
     uint1 counter;
     uint1 initialize;
     uint4 pattern[2 * 32];

     uint1 enable;
     uint4 output;
     uint5 patternaddr;
     uint1 patternbank;
     uint4 patternsample;
    uint32 period;
  } wave;

  struct Noise {
    //noise.cpp
    auto divider() const -> uint;
    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;
    auto read(uint addr) const -> uint8;
    auto write(uint addr, uint8 byte) -> void;
    auto power() -> void;

    Envelope envelope;

     uint6 length;
     uint3 divisor;
     uint1 narrowlfsr;
     uint4 frequency;
     uint1 counter;
     uint1 initialize;

     uint1 enable;
    uint15 lfsr;
     uint4 output;
    uint32 period;
     uint4 volume;
  } noise;

  struct Sequencer {
    //sequencer.cpp
    auto sample() -> void;

    auto read(uint addr) const -> uint8;
    auto write(uint addr, uint8 byte) -> void;
    auto power() -> void;

     uint2 volume;
     uint3 lvolume;
     uint3 rvolume;
     uint1 lenable[4];
     uint1 renable[4];
     uint1 masterenable;

    uint12 base;
     uint3 step;
     int16 lsample;
     int16 rsample;

    uint10 loutput;
    uint10 routput;
  } sequencer;

  struct FIFO {
    //fifo.cpp
    auto sample() -> void;
    auto read() -> void;
    auto write(int8 byte) -> void;
    auto reset() -> void;
    auto power() -> void;

     int8 samples[32];
     int8 active;
     int8 output;

    uint5 rdoffset;
    uint5 wroffset;
    uint6 size;

    uint1 volume;  //0 = 50%, 1 = 100%
    uint1 lenable;
    uint1 renable;
    uint1 timer;
  } fifo[2];
};

extern APU apu;
