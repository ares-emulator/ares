//Sony CXD1222Q-1

struct DSP : Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;
  } debugger;

  n8 apuram[64_KiB];
  n8 registers[128];

  auto mute() const -> bool { return master.mute; }

  //dsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power(bool reset) -> void;

  //memory.cpp
  auto read(n7 address) -> n8;
  auto write(n7 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Envelope { enum : u32 {
    Release,
    Attack,
    Decay,
    Sustain,
  };};

  struct Clock {
    n15 counter;
    n1  sample = 1;
  } clock;

  struct Master {
    n1  reset = 1;
    n1  mute = 1;
    i8  volume[2];
    i17 output[2];
  } master;

  struct Echo {
    i8  feedback;
    i8  volume[2];
    i8  fir[8];
    i16 history[2][8];
    n8  bank;
    n4  delay;
    n1  readonly = 1;
    i17 input[2];
    i17 output[2];

    n8  _bank;
    n1  _readonly;
    n16 _address;
    n16 _offset;  //offset from ESA into echo buffer
    n16 _length;  //number of bytes that echo offset will stop at
    n3  _historyOffset;
  } echo;

  struct Noise {
    n5  frequency;
    n15 lfsr = 0x4000;
  } noise;

  struct BRR {
    n8  bank;

    n8  _bank;
    n8  _source;
    n16 _address;
    n16 _nextAddress;
    n8  _header;
    n8  _byte;
  } brr;

  struct Latch {
    n8  adsr0;
    n8  envx;
    n8  outx;
    n15 pitch;
    i16 output;
  } latch;

  struct Voice {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n7  index;  //voice channel register index: 0x00 for voice 0, 0x10 for voice 1, etc

    i8  volume[2];
    n14 pitch;
    n8  source;
    n8  adsr0;
    n8  adsr1;
    n8  gain;
    n8  envx;
    n1  keyon;
    n1  keyoff;
    n1  modulate;  //0 = normal, 1 = modulate by previous voice pitch
    n1  noise;     //0 = BRR, 1 = noise
    n1  echo;      //0 = direct, 1 = echo
    n1  end;       //0 = keyed on, 1 = BRR end bit encountered

    i16 buffer[12];      //12 decoded samples (mirrored for wrapping)
    n4  bufferOffset;    //place in buffer where next samples will be decoded
    n16 gaussianOffset;  //relative fractional position in sample (0x1000 = 1.0)
    n16 brrAddress;      //address of current BRR block
    n4  brrOffset = 1;   //current decoding offset in BRR block (1-8)
    n3  keyonDelay;      //KON delay/current setup phase
    n2  envelopeMode;
    n11 envelope;        //current envelope level (0-2047)

    //internal latches
    i32 _envelope;       //used by GAIN mode 7, very obscure quirk
    n1  _keylatch;
    n1  _keyon;
    n1  _keyoff;
    n1  _modulate;
    n1  _noise;
    n1  _echo;
    n1  _end;
    n1  _looped;
  } voice[8];

  //gaussian.cpp
  i16 gaussianTable[512];
  auto gaussianConstructTable() -> void;
  auto gaussianInterpolate(const Voice& v) -> s32;

  //counter.cpp
  static const n16 CounterRate[32];
  static const n16 CounterOffset[32];
  auto counterTick() -> void;
  auto counterPoll(u32 rate) -> bool;

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
  auto voiceOutput(Voice& v, n1 channel) -> void;
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
  auto calculateFIR(n1 channel, s32 index) -> s32;
  auto echoOutput(n1 channel) const -> i16;
  auto echoRead(n1 channel) -> void;
  auto echoWrite(n1 channel) -> void;
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
  auto sample(i16 left, i16 right) -> void;
};

extern DSP dsp;
