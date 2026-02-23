struct APU : Thread, IO {
  Node::Object node;
  Node::Audio::Stream stream;

  bool accurate;

  //apu.cpp
  auto setAccurate(bool value) -> void;

  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto sample(u32 channel, n5 index) -> n4;
  auto output() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;
  auto sequencerClear() -> void;
  auto sequencerHeld() -> bool;

  //io.cpp
  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Debugger {
    APU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto ports() -> string;

    struct Properties {
      Node::Debugger::Properties ports;
    } properties;
  } debugger{*this};

  struct DMA {
    //dma.cpp
    auto run() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n20 source;
      n20 length;
      n2  rate;
      n1  hold;
      n1  loop;
      n1  target;
      n1  direction;
      n1  enable;
    } io;

    struct State {
      n32 clock;
      n20 source;
      n20 length;
    } state;
  } dma;

  struct Channel1 {
    //channel1.cpp
    auto tick() -> void;
    auto output() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      n1  enable;
    } io;

    struct State {
      n11 period;
      n5  sampleOffset;
    } state;
  } channel1;

  struct Channel2 {
    //channel2.cpp
    auto tick() -> void;
    auto output() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      n1  enable;
      n1  voice;
      n1  voiceEnableLeftHalf;
      n1  voiceEnableLeftFull;
      n1  voiceEnableRightHalf;
      n1  voiceEnableRightFull;
    } io;

    struct State {
      n11 period;
      n5  sampleOffset;
    } state;
  } channel2;

  struct Channel3 {
    //channel3.cpp
    auto sweep() -> void;
    auto tick() -> void;
    auto output() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      i8  sweepValue;
      n5  sweepTime;
      n1  enable;
      n1  sweep;
    } io;

    struct State {
      n11 period;
      n5  sampleOffset;
      i32 sweepCounter;
    } state;
  } channel3;

  struct Channel4 {
    //channel4.cpp
    auto noiseSample() -> n4;
    auto tick() -> void;
    auto output() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      n3  noiseMode;
      n1  noiseUpdate;
      n1  enable;
      n1  noise;
    } io;

    struct State {
      n11 period;
      n5  sampleOffset;
      n1  noiseOutput;
      n15 noiseLFSR;
    } state;
  } channel4;

  struct Channel5 {
    //channel5.cpp
    auto dmaWrite(n8 sample) -> void;
    auto manualWrite(n8 sample) -> void;
    auto write(n8 sample) -> void;
    auto scale(i8 sample) -> i16;
    auto runOutput() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n2 volume;
      n2 scale;
      n3 speed;
      n1 enable;
      n4 unknown;
      n2 mode;
    } io;

    struct State {
      n32 clock;
      n1 channel;
      n8 left;
      n8 right;
      n1 leftChanged;
      n1 rightChanged;
    } state;

    struct Output {
      i16 left;
      i16 right;
    } output;
  } channel5;

  struct IO {
    n8 waveBase;
    n1 speakerEnable;
    n2 speakerShift;
    n1 headphonesEnable;
    n1 headphonesConnected;
    n2 masterVolume;

    n1 seqDbgHold;
    n1 seqDbgOutputForce55;
    n1 seqDbgChForce4;
    n1 seqDbgChForce2;
    n1 seqDbgSweepClock;
    n2 seqDbgNoise;
    n1 seqDbgUnknown;
    
    // This output covers Channels 1-4 (excluding Hyper Voice)
    struct Output {
      n10 left;
      n10 right;
    } output;
  } io;

  struct State {
    n13 sweepClock;
    n7 apuClock;
  } state;
};

extern APU apu;
