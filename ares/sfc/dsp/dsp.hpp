//Sony CXD1222Q-1

struct DSP : Thread {
  Node::Component node;
  Node::Stream stream;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory ram;
    } memory;
  } debugger;

  uint8 apuram[64 * 1024];
  uint8 registers[128];

  auto mute() const -> bool { return master.mute; }

  //dsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power(bool reset) -> void;

  //memory.cpp
  auto read(uint7 address) -> uint8;
  auto write(uint7 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Envelope { enum : uint {
    Release,
    Attack,
    Decay,
    Sustain,
  };};

  struct Clock {
    uint15 counter;
     uint1 sample = 1;
  } clock;

  struct Master {
    uint1 reset = 1;
    uint1 mute = 1;
     int8 volume[2];
    int17 output[2];
  } master;

  struct Echo {
     int8 feedback;
     int8 volume[2];
     int8 fir[8];
    int16 history[2][8];
    uint8 bank;
    uint4 delay;
    uint1 readonly = 1;
    int17 input[2];
    int17 output[2];

     uint8 _bank;
     uint1 _readonly;
    uint16 _address;
    uint16 _offset;  //offset from ESA into echo buffer
    uint16 _length;  //number of bytes that echo offset will stop at
     uint3 _historyOffset;
  } echo;

  struct Noise {
     uint5 frequency;
    uint15 lfsr = 0x4000;
  } noise;

  struct BRR {
     uint8  bank;

     uint8 _bank;
     uint8 _source;
    uint16 _address;
    uint16 _nextAddress;
     uint8 _header;
     uint8 _byte;
  } brr;

  struct Latch {
     uint8 adsr0;
     uint8 envx;
     uint8 outx;
    uint15 pitch;
     int16 output;
  } latch;

  struct Voice {
    //serialization.cpp
    auto serialize(serializer&) -> void;

     uint7 index;  //voice channel register index: 0x00 for voice 0, 0x10 for voice 1, etc

      int8 volume[2];
    uint14 pitch;
     uint8 source;
     uint8 adsr0;
     uint8 adsr1;
     uint8 gain;
     uint8 envx;
     uint1 keyon;
     uint1 keyoff;
     uint1 modulate;  //0 = normal, 1 = modulate by previous voice pitch
     uint1 noise;     //0 = BRR, 1 = noise
     uint1 echo;      //0 = direct, 1 = echo
     uint1 end;       //0 = keyed on, 1 = BRR end bit encountered

     int16 buffer[12];      //12 decoded samples (mirrored for wrapping)
     uint4 bufferOffset;    //place in buffer where next samples will be decoded
    uint16 gaussianOffset;  //relative fractional position in sample (0x1000 = 1.0)
    uint16 brrAddress;      //address of current BRR block
     uint4 brrOffset = 1;   //current decoding offset in BRR block (1-8)
     uint3 keyonDelay;      //KON delay/current setup phase
     uint2 envelopeMode;
    uint11 envelope;        //current envelope level (0-2047)

    //internal latches
     int32 _envelope;       //used by GAIN mode 7, very obscure quirk
     uint1 _keylatch;
     uint1 _keyon;
     uint1 _keyoff;
     uint1 _modulate;
     uint1 _noise;
     uint1 _echo;
     uint1 _end;
     uint1 _looped;
  } voice[8];

  //gaussian.cpp
  int16 gaussianTable[512];
  auto gaussianConstructTable() -> void;
  auto gaussianInterpolate(const Voice& v) -> int;

  //counter.cpp
  static const uint16 CounterRate[32];
  static const uint16 CounterOffset[32];
  auto counterTick() -> void;
  auto counterPoll(uint rate) -> bool;

  //envelope.cpp
  auto envelopeRun(Voice& v) -> void;

  //brr.cpp
  auto brrDecode(Voice& v) -> void;

  //misc.cpp
  auto misc27() -> void;
  auto misc28() -> void;
  auto misc29() -> void;
  auto misc30() -> void;

  //voice.cpp
  auto voiceOutput(Voice& v, uint1 channel) -> void;
  auto voice1 (Voice& v) -> void;
  auto voice2 (Voice& v) -> void;
  auto voice3 (Voice& v) -> void;
  auto voice3a(Voice& v) -> void;
  auto voice3b(Voice& v) -> void;
  auto voice3c(Voice& v) -> void;
  auto voice4 (Voice& v) -> void;
  auto voice5 (Voice& v) -> void;
  auto voice6 (Voice& v) -> void;
  auto voice7 (Voice& v) -> void;
  auto voice8 (Voice& v) -> void;
  auto voice9 (Voice& v) -> void;

  //echo.cpp
  auto calculateFIR(uint1 channel, int index) -> int;
  auto echoOutput(uint1 channel) const -> int16;
  auto echoRead(uint1 channel) -> void;
  auto echoWrite(uint1 channel) -> void;
  auto echo22() -> void;
  auto echo23() -> void;
  auto echo24() -> void;
  auto echo25() -> void;
  auto echo26() -> void;
  auto echo27() -> void;
  auto echo28() -> void;
  auto echo29() -> void;
  auto echo30() -> void;

  //dsp.cpp
  auto tick() -> void;
  auto sample(int16 left, int16 right) -> void;
};

extern DSP dsp;
