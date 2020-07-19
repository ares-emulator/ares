//Sound Processing Unit

struct SPU : Thread {
  Node::Component node;
  Node::Stream stream;
  Memory::Writable ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory ram;
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
  auto fifoManualWrite() -> void;

  //adsr.cpp
  auto adsrConstructTable() -> void;

  //gaussian.cpp
  auto gaussianConstructTable() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Master {
    uint1 enable;
    uint1 mute;  //0 = muted; 1 = unmuted
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
    i32 ticks;
    i32 step;
  } adsrEntries[2][128];

  struct Transfer {
     uint2 mode = 0;
     uint3 type = 0;
    uint16 address = 0;
    uint19 current = 0;
     uint1 unknown_0 = 0;
    uint12 unknown_4_15 = 0;
  } transfer;

  struct IRQ {
     uint1 enable;
     uint1 flag;
    uint16 address;
  } irq;

  struct Volume {
     uint1 mode = 0;  //0 = volume level; 1 = sweep volume
    uint14 level = 0;
  } volume[2];

  struct Envelope {
    //envelope.cpp
    auto reset(u8 rate, bool decreasing, bool exponential) -> void;
    auto tick(i32 level) -> i16;

    int32 counter;
    uint8 rate;
    uint1 decreasing;
    uint1 exponential;
  } envelope;

  struct Sweep {
    uint1 mode = 0;       //0 = linear; 1 = exponential
    uint1 direction = 0;  //0 = increase; 1 = decrease
    uint1 phase = 0;      //0 = positive; 1 = negative
    uint5 shift = 0;
    uint2 step = 0;
    uint5 unknown = 0;
  } sweep[2];

  struct CDDA {
     uint1 enable = 0;
     uint1 reverb = 0;
    uint16 volume[2];
  } cdda;

  struct External {
     uint1 enable = 0;
     uint1 reverb = 0;
    uint16 volume[2];
  } external;

  struct Current {
    uint16 volume[2];
  } current;

  struct Reverb {
    SPU& self;
    Reverb(SPU& self) : self(self) {}

    //reverb.cpp
    auto process(i16 linput, i16 rinput) -> std::pair<i32, i32>;
    auto compute() -> void;
    auto iiasm(i16 sample) -> i32;
    template<bool phase> auto r2244(const int16* source) -> i32;
    auto r4422(const int16* source) -> i32;
    auto address(u32 address) -> u32;
    auto read(u32 address, i32 offset = 0) -> i16;
    auto write(u32 address, i16 data) -> void;

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

    static constexpr i16 coefficients[20] = {
      -1, +2, -10, +35, -103, +266, -616, +1332, -2960, +10246,
      +10246, -2960, +1332, -616, +266, -103, +35, -10, +2, -1,
    };
  } reverb{*this};

  struct Voice {
    SPU& self;
    const uint id;
    Voice(SPU& self, uint id) : self(self), id(id) {}

    //voice.app
    auto sample(i16 modulation) -> std::pair<i32, i32>;
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
    auto gaussianRead(i8 index) const -> i32;
    auto gaussianInterpolate() const -> i32;

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
      int16 lastVolume;
      int16 target;
    } adsr;

    struct Current {
      uint16 volume[2];
    } current;

    Volume volume[2];
    Sweep sweep[2];
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

  queue<u16> fifo;

//unserialized:
  i16 gaussianTable[512];
};

extern SPU spu;
