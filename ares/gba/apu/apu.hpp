struct APU : Thread, IO {
  Node::Object node;
  Node::Audio::Stream stream;

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto readIO(n32 address) -> n8;
  auto writeIO(n32 address, n8 byte) -> void;
  auto power() -> void;

  //sequencer.cpp
  auto sequence() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  u32 clock;

  struct Bias {
    n10 level = 0x200;
    n2  amplitude = 0;
  } bias;

  struct Sweep {
    n3 shift;
    n1 direction;
    n3 frequency;

    n1 enable;
    n1 negate;
    n3 period;
  };

  struct Envelope {
    auto dacEnable() const -> bool { return volume || direction; }

    n3 frequency;
    n1 direction;
    n4 volume;

    n3 period;
  };

  struct Square {
    //square.cpp
    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;

    Envelope envelope;

    n1  enable;
    n6  length;
    n2  duty;
    n11 frequency;
    n1  counter;
    n1  initialize;

    i32 shadowfrequency;
    n1  signal;
    n4  output;
    n32 period;
    n3  phase;
    n4  volume;
  };

  struct Square1 : Square {
    //square1.cpp
    auto runSweep(bool update) -> void;
    auto clockSweep() -> void;
    auto read(u32 address) const -> n8;
    auto write(u32 address, n8 byte) -> void;
    auto power() -> void;

    Sweep sweep;
  } square1;

  struct Square2 : Square {
    //square2.cpp
    auto read(u32 address) const -> n8;
    auto write(u32 address, n8 byte) -> void;
    auto power() -> void;
  } square2;

  struct Wave {
    //wave.cpp
    auto run() -> void;
    auto clockLength() -> void;
    auto read(u32 address) const -> n8;
    auto write(u32 address, n8 byte) -> void;
    auto readRAM(u32 address) const -> n8;
    auto writeRAM(u32 address, n8 byte) -> void;
    auto power() -> void;

    n1  mode;
    n1  bank;
    n1  dacenable;
    n8  length;
    n3  volume;
    n11 frequency;
    n1  counter;
    n1  initialize;
    n4  pattern[2 * 32];

    n1  enable;
    n4  output;
    n5  patternaddr;
    n1  patternbank;
    n4  patternsample;
    n32 period;
  } wave;

  struct Noise {
    //noise.cpp
    auto divider() const -> u32;
    auto run() -> void;
    auto clockLength() -> void;
    auto clockEnvelope() -> void;
    auto read(u32 address) const -> n8;
    auto write(u32 address, n8 byte) -> void;
    auto power() -> void;

    Envelope envelope;

    n6  length;
    n3  divisor;
    n1  narrowlfsr;
    n4  frequency;
    n1  counter;
    n1  initialize;

    n1  enable;
    n15 lfsr;
    n4  output;
    n32 period;
    n4  volume;
  } noise;

  struct Sequencer {
    //sequencer.cpp
    auto sample() -> void;

    auto read(u32 address) const -> n8;
    auto write(u32 address, n8 byte) -> void;
    auto power() -> void;

    n2  volume;
    n3  lvolume;
    n3  rvolume;
    n1  lenable[4];
    n1  renable[4];
    n1  masterenable;

    n12 base;
    n3  step;
    i16 lsample;
    i16 rsample;

    n10 loutput;
    n10 routput;
  } sequencer;

  struct FIFO {
    //fifo.cpp
    auto sample() -> void;
    auto read() -> void;
    auto write(i8 byte) -> void;
    auto reset() -> void;
    auto power() -> void;

    i8 samples[32];
    i8 active;
    i8 output;

    n5 rdoffset;
    n5 wroffset;
    n6 size;

    n1 volume;  //0 = 50%, 1 = 100%
    n1 lenable;
    n1 renable;
    n1 timer;
  } fifo[2];
};

extern APU apu;
