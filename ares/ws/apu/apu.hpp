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
  auto portRead(n16 address) -> n8;
  auto portWrite(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct State {
    n13 sweepClock;
  } s;

  struct Registers {
    //$008f  SND_WAVE_BASE
    n8 waveBase;

    //$0091  SND_OUTPUT
    n1 speakerEnable;
    n2 speakerShift;
    n1 headphonesEnable;
    n1 headphonesConnected;

    //$009e  SND_VOLUME
    n2 masterVolume;
  } r;

  struct DMA {
    auto run() -> void;

    struct State {
      u32 clock;
      n20 source;
      n20 length;
    } s;

    struct Registers {
      //$004a-$004c  SDMA_SRC
      n20 source;

      //$004e-$0050  SDMA_LEN
      n20 length;

      //$0052  SDMA_CTRL
      n2 rate;
      n1 unknown;
      n1 loop;
      n1 target;
      n1 direction;
      n1 enable;
    } r;
  } dma;

  struct Channel1 {
    auto run() -> void;

    struct Output {
      n8 left;
      n8 right;
    } o;

    struct State {
      n11 period;
      n5  sampleOffset;
    } s;

    struct Registers {
      //$0080-0081  SND_CH1_PITCH
      n11 pitch;

      //$0088  SND_CH1_VOL
      n4 volumeLeft;
      n4 volumeRight;

      //$0090  SND_CTRL
      n1 enable;
    } r;
  } channel1;

  struct Channel2 {
    auto run() -> void;

    struct Output {
      n8 left;
      n8 right;
    } o;

    struct State {
      n11 period;
      n5  sampleOffset;
    } s;

    struct Registers {
      //$0082-0083  SND_CH2_PITCH
      n11 pitch;

      //$0089  SND_CH2_VOL
      n4 volumeLeft;
      n4 volumeRight;

      //$0090  SND_CTRL
      n1 enable;
      n1 voice;

      //$0094  SND_VOICE_CTRL
      n2 voiceEnableLeft;
      n2 voiceEnableRight;
    } r;
  } channel2;

  struct Channel3 {
    auto sweep() -> void;
    auto run() -> void;

    struct Output {
      n8 left;
      n8 right;
    } o;

    struct State {
      n11 period;
      n5  sampleOffset;

      s32 sweepCounter;
    } s;

    struct Registers {
      //$0084-0085  SND_CH3_PITCH
      n11 pitch;

      //$008a  SND_CH3_VOL
      n4 volumeLeft;
      n4 volumeRight;

      //$008c  SND_SWEEP_VALUE
      i8 sweepValue;

      //$008d  SND_SWEEP_TIME
      n5 sweepTime;

      //$0090  SND_CTRL
      n1 enable;
      n1 sweep;
    } r;
  } channel3;

  struct Channel4 {
    auto noiseSample() -> n4;
    auto run() -> void;

    struct Output {
      n8 left;
      n8 right;
    } o;

    struct State {
      n11 period;
      n5  sampleOffset;

      n1  noiseOutput;
      n15 noiseLFSR;
    } s;

    struct Registers {
      //$0086-0087  SND_CH4_PITCH
      n11 pitch;

      //$008b  SND_CH4_VOL
      n4 volumeLeft;
      n4 volumeRight;

      //$008e  SND_NOISE
      n3 noiseMode;
      n1 noiseReset;
      n1 noiseUpdate;

      //$0090  SND_CTRL
      n1 enable;
      n1 noise;
    } r;
  } channel4;

  struct Channel5 {
    auto run() -> void;

    struct Output {
      i11 left;
      i11 right;
    } o;

    struct State {
      u32 clock;
      i8  data;
    } s;

    struct Registers {
      //$006a  HYPER_CTRL
      n2 volume;
      n2 scale;
      n3 speed;
      n1 enable;

      //$006b  HYPER_CHAN_CTRL
      n4 unknown;
      n1 leftEnable;
      n1 rightEnable;
    } r;
  } channel5;
};

extern APU apu;
