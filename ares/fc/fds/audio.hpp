struct FDSAudio {
  Node::Audio::Stream stream;

  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto clock() -> void;
  auto updateOutput() -> void;
  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Operator {
    auto clockEnvelope() -> bool;
    auto reloadPeriod() -> void;

    n8  masterSpeed = 0xff;
    n6  speed;
    n6  gain;
    n1  direction;  //0 = decrease, 1 = increase
    n1  envelope;   //0 = disable,  1 = enable
    n12 frequency;
    n32 period;
  };

  struct Modulator : Operator {
    auto enabled() -> bool;
    auto clockModulator() -> bool;
    auto updateOutput(n16 pitch) -> void;
    auto updateCounter(i8 value) -> void;

    struct Table {
      n3 data[64];
      n6 index;
    };

    n1  disabled;
    i8  counter;
    n16 overflow;
    i32 output;
    Table table;
  };

  struct Waveform {
    n1  halt;
    n1  writable;
    n16 overflow;
    n6  data[64];
    n6  index;
  };

  n1 enable;
  n1 envelopes;  //0 = disable, 1 = enable
  n2 masterVolume;
  Operator carrier;
  Modulator modulator;
  Waveform waveform;
};
