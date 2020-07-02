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
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //fifo.cpp
  auto fifoReset() -> void;
  auto fifoWrite(u16 data) -> void;
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
    uint19 address = 0;
    uint19 irqAddress = 0;
     uint1 irqEnable = 0;
     uint1 unknown_0 = 0;
    uint12 unknown_4_15 = 0;
  } transfer;

  struct FIFO {
    uint16 data[32];
    uint16 size;
  } fifo;

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
    uint1 enable;

    struct Volume {
      uint16 output[2];
      uint16 reflection[2];
      uint16 comb[4];
      uint16 apf[2];
      uint16 input[2];
    } volume;

    struct Offset {
      uint19 apf1 = 0;
      uint19 apf2 = 0;
    } offset;

    struct Address {
      uint19 work = 0;
      uint19 apf1[2];
      uint19 apf2[2];
      uint19 same1[2];
      uint19 same2[2];
      uint19 diff1[2];
      uint19 diff2[2];
      uint19 comb1[2];
      uint19 comb2[2];
      uint19 comb3[2];
      uint19 comb4[2];
    } address;
  } reverb;

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

//unserialized:
  i16 gaussianTable[512];
};

extern SPU spu;
