//Sound Processing Unit

struct SPU : Thread, Memory::Interface {
  Node::Object node;
  Node::Audio::Stream stream;
  Memory::Writable ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;
  } debugger;

  //spu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto sample() -> void;
  auto step(u32 clocks) -> void;

  auto power(bool reset) -> void;

  //io.cpp
  auto readRAM(u32 address) -> u16;
  auto writeRAM(u32 address, u16 data) -> void;
  auto readDMA() -> u32;
  auto writeDMA(u32 data) -> void;
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //fifo.cpp
  auto fifoReadBlock() -> void;
  auto fifoWriteBlock() -> void;

  //capture.cpp
  auto captureVolume(u32 channel, s16 volume) -> void;

  //adsr.cpp
  auto adsrConstructTable() -> void;

  //gaussian.cpp
  auto gaussianConstructTable() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Master {
    n1 enable;
    n1 unmute;
  } master;

  struct Noise {
    SPU& self;
    Noise(SPU& self) : self(self) {}

    //noise.cpp
    auto update() -> void;

    n2  step;
    n4  shift;
  //internal:
    n32 level;
    n32 count;
  } noise{*this};

  struct ADSREntry {
  //unserialized:
    s32 ticks;
    s32 step;
  } adsrEntries[2][128];

  struct Transfer {
    n2  mode;
    n3  type;
    n16 address;
    n19 current;
    n1  unknown_0;
    n12 unknown_4_15;
  } transfer;

  struct IRQ {
    n1  enable;
    n1  flag;
    n16 address;
  } irq;

  struct Envelope {
    //envelope.cpp
    auto reset(u8 rate, bool decreasing, bool exponential) -> void;
    auto tick(s32 level) -> s16;

    i32 counter;
    n7  rate;
    n1  decreasing;
    n1  exponential;
  } envelope;

  struct VolumeSweep : Envelope {
    //envelope.cpp
    auto reset() -> void;
    auto tick() -> void;

    n1  active;
    n1  sweep;
    n1  negative;
    i15 level;
    i16 current;
  } volume[2];

  struct CDAudio {
    n1  enable;
    n1  reverb;
    n16 volume[2];
  } cdaudio;

  struct External {
    n1  enable;
    n1  reverb;
    n16 volume[2];
  } external;

  struct Current {
    n16 volume[2];
  } current;

  struct Reverb {
    SPU& self;
    Reverb(SPU& self) : self(self) {}

    //reverb.cpp
    auto process(s16 linput, s16 rinput) -> std::pair<s32, s32>;
    auto compute() -> void;
    auto iiasm(s16 sample) -> s32;
    template<bool phase> auto r2244(const i16* source) -> s32;
    auto r4422(const i16* source) -> s32;
    auto address(u32 address) -> u32;
    auto read(u32 address, s32 offset = 0) -> s16;
    auto write(u32 address, s16 data) -> void;

    n1  enable;

    n16 vLOUT;
    n16 vROUT;
    n16 mBASE;

    n16 FB_SRC_A;
    n16 FB_SRC_B;
    i16 IIR_ALPHA;
    i16 ACC_COEF_A;
    i16 ACC_COEF_B;
    i16 ACC_COEF_C;
    i16 ACC_COEF_D;
    i16 IIR_COEF;
    i16 FB_ALPHA;
    i16 FB_X;
    n16 IIR_DEST_A0;
    n16 IIR_DEST_A1;
    n16 ACC_SRC_A0;
    n16 ACC_SRC_A1;
    n16 ACC_SRC_B0;
    n16 ACC_SRC_B1;
    n16 IIR_SRC_A0;
    n16 IIR_SRC_A1;
    n16 IIR_DEST_B0;
    n16 IIR_DEST_B1;
    n16 ACC_SRC_C0;
    n16 ACC_SRC_C1;
    n16 ACC_SRC_D0;
    n16 ACC_SRC_D1;
    n16 IIR_SRC_B1;  //misordered
    n16 IIR_SRC_B0;  //misordered
    n16 MIX_DEST_A0;
    n16 MIX_DEST_A1;
    n16 MIX_DEST_B0;
    n16 MIX_DEST_B1;
    i16 IN_COEF_L;
    i16 IN_COEF_R;

  //internal:
    i16 lastInput[2];
    i32 lastOutput[2];
    i16 downsampleBuffer[2][128];
    i16 upsampleBuffer[2][64];
    n6  resamplePosition;
    n18 baseAddress;
    n18 currentAddress;

    static constexpr s16 coefficients[20] = {
      -1, +2, -10, +35, -103, +266, -616, +1332, -2960, +10246,
      +10246, -2960, +1332, -616, +266, -103, +35, -10, +2, -1,
    };
  } reverb{*this};

  struct Voice {
    SPU& self;
    const u32 id;
    Voice(SPU& self, u32 id) : self(self), id(id) {}

    //voice.app
    auto sample(s32 modulation) -> std::pair<s32, s32>;
    auto tickEnvelope() -> void;
    auto advancePhase() -> void;
    auto updateEnvelope() -> void;
    auto forceOff() -> void;
    auto keyOff() -> void;
    auto keyOn() -> void;

    //adpcm.cpp
    auto readBlock() -> void;
    auto decodeBlock() -> void;

    //gaussian.cpp
    auto gaussianRead(s8 index) const -> s32;
    auto gaussianInterpolate() const -> s32;

    struct ADPCM {
      n19 startAddress;
      n19 repeatAddress;
      n16 sampleRate;
    //internal:
      n19 currentAddress;
      n1  hasSamples;
      n1  ignoreLoopAddress;
      i16 lastSamples[2];
      i16 previousSamples[3];
      i16 currentSamples[28];
    } adpcm;

    struct Block {
      //header
      n4 shift;
      n3 filter;

      //flags
      n1 loopEnd;
      n1 loopRepeat;
      n1 loopStart;

      //samples
      n4 brr[28];
    } block;

    struct Attack {
      n7 rate;
      n1 exponential;
    } attack;

    struct Decay {
      n4 rate;
    } decay;

    struct Sustain {
      n4 level;
      n1 exponential;
      n1 decrease;
      n7 rate;
      n1 unknown;
    } sustain;

    struct Release {
      n1 exponential;
      n5 rate;
    } release;

    struct ADSR {
      enum class Phase : u32 { Off, Attack, Decay, Sustain, Release };
      Phase phase = Phase::Off;
      i16 volume;
    //internal:
      i32 lastVolume;
      i16 target;
    } adsr;

    struct Current {
      n16 volume[2];
    } current;

    VolumeSweep volume[2];
    Envelope envelope;
    n32 counter;
    n1  pmon;
    n1  non;
    n1  eon;
    n1  kon;
    n1  koff;
    n1  endx;
  } voice[24] = {
    {*this,  0}, {*this,  1}, {*this,  2}, {*this,  3},
    {*this,  4}, {*this,  5}, {*this,  6}, {*this,  7},
    {*this,  8}, {*this,  9}, {*this, 10}, {*this, 11},
    {*this, 12}, {*this, 13}, {*this, 14}, {*this, 15},
    {*this, 16}, {*this, 17}, {*this, 18}, {*this, 19},
    {*this, 20}, {*this, 21}, {*this, 22}, {*this, 23},
  };

  struct Capture {
    n10 address;
  } capture;

  queue<u16[32]> fifo;

//unserialized:
  s16 gaussianTable[512];
};

extern SPU spu;
