//Sound Processing Unit

struct SPU : Thread {
  Node::Component node;
  Node::Stream stream;
  Memory::Writable ram;

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

  //gaussian.cpp
  auto gaussianConstructTable() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint1 cdAudioEnable = 0;
    uint1 cdAudioReverb = 0;
    uint1 externalAudioEnable = 0;
    uint1 externalAudioReverb = 0;

    struct RAMTransfer {
       uint2 mode = 0;
       uint3 type = 0;
      uint16 address = 0;
      uint16 irqAddress = 0;
       uint1 unknown_0 = 0;
      uint12 unknown_4_15 = 0;
    } ramTransfer;
  } io;

  struct Volume {
     uint1 mode = 0;  //0 = volume level; 1 = sweep volume
    uint14 level = 0;
  } volume[2];

  struct Sweep {
    uint1 mode = 0;       //0 = linear; 1 = exponential
    uint1 direction = 0;  //0 = increase; 1 = decrease
    uint1 phase = 0;      //0 = positive; 1 = negative
    uint5 shift = 0;
    uint2 step = 0;
    uint5 unknown = 0;
  } sweep[2];

  struct CDDA {
    uint16 volume[2];
  } cdda;

  struct External {
    uint16 volume[2];
  } external;

  struct Current {
    uint16 volume[2];
  } current;

  struct Reverb {
    struct Volume {
      uint16 output[2];
      uint16 reflection[2];
      uint16 comb[4];
      uint16 apf[2];
      uint16 input[2];
    } volume;

    struct Offset {
      uint16 apf1 = 0;
      uint16 apf2 = 0;
    } offset;

    struct Address {
      uint16 work = 0;
      uint16 apf1[2];
      uint16 apf2[2];
      uint16 same1[2];
      uint16 same2[2];
      uint16 diff1[2];
      uint16 diff2[2];
      uint16 comb1[2];
      uint16 comb2[2];
      uint16 comb3[2];
      uint16 comb4[2];
    } address;
  } reverb;

  struct Voice {
    struct ADPCM {
      uint16 startAddress = 0;
      uint16 repeatAddress = 0;
      uint16 sampleRate = 0;
    } adpcm;

    struct Attack {
      uint1 mode = 0;  //0 = linear; 1 = exponential
      uint5 shift = 0;
      uint2 step = 0;
    } attack;

    struct Decay {
      uint4 shift = 0;
    } decay;

    struct Sustain {
      uint4 level = 0;
      uint1 mode = 0;
      uint1 direction = 0;  //0 = increase; 1 = decrease
      uint5 shift = 0;
      uint2 step = 0;
      uint1 unknown = 0;
    } sustain;

    struct Release {
      uint1 mode = 0;
      uint5 shift = 0;
    } release;

    struct ADSR {
      uint16 volume = 0;
    } adsr;

    struct Current {
      uint16 volume[2];
    } current;

    Volume volume[2];
    Sweep sweep[2];
    uint1 pmon = 0;
    uint1 non = 0;
    uint1 eon = 0;
    uint1 kon = 0;
    uint1 koff = 0;
    uint1 endx = 0;
  } voice[24];

//unserialized:
  i16 gaussianTable[512];
};

extern SPU spu;
