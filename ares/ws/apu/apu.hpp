struct APU : Thread, IO {
  Node::Object node;
  Node::Audio::Stream stream;

  //apu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto sample(u32 channel, n5 index) -> n4;
  auto dacRun() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(n16 address) -> n8;
  auto writeIO(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

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
      n1  unknown;
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
    auto run() -> void;
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

    struct Output {
      n8 left;
      n8 right;
    } output;
  } channel1;

  struct Channel2 {
    //channel2.cpp
    auto run() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      n1  enable;
      n1  voice;
      n2  voiceEnableLeft;
      n2  voiceEnableRight;
    } io;

    struct State {
      n11 period;
      n5  sampleOffset;
    } state;

    struct Output {
      n8 left;
      n8 right;
    } output;
  } channel2;

  struct Channel3 {
    //channel3.cpp
    auto sweep() -> void;
    auto run() -> void;
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

    struct Output {
      n8 left;
      n8 right;
    } output;
  } channel3;

  struct Channel4 {
    //channel4.cpp
    auto noiseSample() -> n4;
    auto run() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n11 pitch;
      n4  volumeLeft;
      n4  volumeRight;
      n3  noiseMode;
      n1  noiseReset;
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

    struct Output {
      n8 left;
      n8 right;
    } output;
  } channel4;

  struct Channel5 {
    //channel5.cpp
    auto run() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n2 volume;
      n2 scale;
      n3 speed;
      n1 enable;
      n4 unknown;
      n1 leftEnable;
      n1 rightEnable;
    } io;

    struct State {
      n32 clock;
      i8  data;
    } state;

    struct Output {
      i11 left;
      i11 right;
    } output;
  } channel5;

  struct IO {
    n8 waveBase;
    n1 speakerEnable;
    n2 speakerShift;
    n1 headphonesEnable;
    n1 headphonesConnected;
    n2 masterVolume;
  } io;

  struct State {
    n13 sweepClock;
  } state;
};

extern APU apu;
