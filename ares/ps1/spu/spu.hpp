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
  auto step(uint clocks) -> void;

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
  auto captureVolume(uint channel, s16 volume) -> void;

  //adsr.cpp
  auto adsrConstructTable() -> void;

  //gaussian.cpp
  auto gaussianConstructTable() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Master {
    uint1 enable;
    uint1 unmute;
  } master;

  struct Noise {
    SPU& self;
    Noise(SPU& self) : self(self) {}

    //noise.cpp
    auto update() -> void;

     uint2 step;
     uint4 shift;
  //internal:
    uint32 level;
    uint32 count;
  } noise{*this};

  struct ADSREntry {
  //unserialized:
    s32 ticks;
    s32 step;
  } adsrEntries[2][128];

  struct Transfer {
     uint2 mode;
     uint3 type;
    uint16 address;
    uint19 current;
     uint1 unknown_0;
    uint12 unknown_4_15;
  } transfer;

  struct IRQ {
     uint1 enable;
     uint1 flag;
    uint16 address;
  } irq;

  struct Envelope {
    //envelope.cpp
    auto reset(u8 rate, bool decreasing, bool exponential) -> void;
    auto tick(s32 level) -> s16;

    int32 counter;
    uint7 rate;
    uint1 decreasing;
    uint1 exponential;
  } envelope;

  struct VolumeSweep : Envelope {
    //envelope.cpp
    auto reset() -> void;
    auto tick() -> void;

    uint1 active;
    uint1 sweep;
    uint1 negative;
    int15 level;
    int16 current;
  } volume[2];

  struct CDAudio {
     uint1 enable;
     uint1 reverb;
    uint16 volume[2];
  } cdaudio;

  struct External {
     uint1 enable;
     uint1 reverb;
    uint16 volume[2];
  } external;

  struct Current {
    uint16 volume[2];
  } current;

  struct Reverb {
    SPU& self;
    Reverb(SPU& self) : self(self) {}

    //reverb.cpp
    auto process(s16 linput, s16 rinput) -> std::pair<s32, s32>;
    auto compute() -> void;
    auto iiasm(s16 sample) -> s32;
    template<bool phase> auto r2244(const int16* source) -> s32;
    auto r4422(const int16* source) -> s32;
    auto address(u32 address) -> u32;
    auto read(u32 address, s32 offset = 0) -> s16;
    auto write(u32 address, s16 data) -> void;

     uint1 enable;

    uint16 vLOUT;
    uint16 vROUT;
    uint16 mBASE;

    uint16 FB_SRC_A;
    uint16 FB_SRC_B;
     int16 IIR_ALPHA;
     int16 ACC_COEF_A;
     int16 ACC_COEF_B;
     int16 ACC_COEF_C;
     int16 ACC_COEF_D;
     int16 IIR_COEF;
     int16 FB_ALPHA;
     int16 FB_X;
    uint16 IIR_DEST_A0;
    uint16 IIR_DEST_A1;
    uint16 ACC_SRC_A0;
    uint16 ACC_SRC_A1;
    uint16 ACC_SRC_B0;
    uint16 ACC_SRC_B1;
    uint16 IIR_SRC_A0;
    uint16 IIR_SRC_A1;
    uint16 IIR_DEST_B0;
    uint16 IIR_DEST_B1;
    uint16 ACC_SRC_C0;
    uint16 ACC_SRC_C1;
    uint16 ACC_SRC_D0;
    uint16 ACC_SRC_D1;
    uint16 IIR_SRC_B1;  //misordered
    uint16 IIR_SRC_B0;  //misordered
    uint16 MIX_DEST_A0;
    uint16 MIX_DEST_A1;
    uint16 MIX_DEST_B0;
    uint16 MIX_DEST_B1;
     int16 IN_COEF_L;
     int16 IN_COEF_R;

  //internal:
     int16 lastInput[2];
     int32 lastOutput[2];
     int16 downsampleBuffer[2][128];
     int16 upsampleBuffer[2][64];
     uint6 resamplePosition;
    uint18 baseAddress;
    uint18 currentAddress;

    static constexpr s16 coefficients[20] = {
      -1, +2, -10, +35, -103, +266, -616, +1332, -2960, +10246,
      +10246, -2960, +1332, -616, +266, -103, +35, -10, +2, -1,
    };
  } reverb{*this};

  struct Voice {
    SPU& self;
    const uint id;
    Voice(SPU& self, uint id) : self(self), id(id) {}

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
      uint19 startAddress;
      uint19 repeatAddress;
      uint16 sampleRate;
    //internal:
      uint19 currentAddress;
       uint1 hasSamples;
       uint1 ignoreLoopAddress;
       int16 lastSamples[2];
       int16 previousSamples[3];
       int16 currentSamples[28];
    } adpcm;

    struct Block {
      //header
      uint4 shift;
      uint3 filter;

      //flags
      uint1 loopEnd;
      uint1 loopRepeat;
      uint1 loopStart;

      //samples
      uint4 brr[28];
    } block;

    struct Attack {
      uint7 rate;
      uint1 exponential;
    } attack;

    struct Decay {
      uint4 rate;
    } decay;

    struct Sustain {
      uint4 level;
      uint1 exponential;
      uint1 decrease;
      uint7 rate;
      uint1 unknown;
    } sustain;

    struct Release {
      uint1 exponential;
      uint5 rate;
    } release;

    struct ADSR {
      enum class Phase : uint { Off, Attack, Decay, Sustain, Release };
      Phase phase = Phase::Off;
      int16 volume;
    //internal:
      int32 lastVolume;
      int16 target;
    } adsr;

    struct Current {
      uint16 volume[2];
    } current;

    VolumeSweep volume[2];
    Envelope envelope;
    uint32 counter;
     uint1 pmon;
     uint1 non;
     uint1 eon;
     uint1 kon;
     uint1 koff;
     uint1 endx;
  } voice[24] = {
    {*this,  0}, {*this,  1}, {*this,  2}, {*this,  3},
    {*this,  4}, {*this,  5}, {*this,  6}, {*this,  7},
    {*this,  8}, {*this,  9}, {*this, 10}, {*this, 11},
    {*this, 12}, {*this, 13}, {*this, 14}, {*this, 15},
    {*this, 16}, {*this, 17}, {*this, 18}, {*this, 19},
    {*this, 20}, {*this, 21}, {*this, 22}, {*this, 23},
  };

  struct Capture {
    uint10 address;
  } capture;

  queue<u16[32]> fifo;

//unserialized:
  s16 gaussianTable[512];
};

extern SPU spu;
