struct FDSAudio {
  Node::Stream stream;

  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto clock() -> void;
  auto updateOutput() -> void;
  auto read(uint16 address, uint8 data) -> uint8;
  auto write(uint16 address, uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Operator {
    auto clockEnvelope() -> bool;
    auto reloadPeriod() -> void;

     uint8 masterSpeed = 0xff;
     uint6 speed;
     uint6 gain;
     uint1 direction;  //0 = decrease, 1 = increase
     uint1 envelope;   //0 = disable,  1 = enable
    uint12 frequency;
    uint32 period;
  };

  struct Modulator : Operator {
    auto enabled() -> bool;
    auto clockModulator() -> bool;
    auto updateOutput(uint16 pitch) -> void;
    auto updateCounter(int8 value) -> void;

    struct Table {
      uint3 data[64];
      uint6 index;
    };

     uint1 disabled;
      int8 counter;
    uint16 overflow;
     int32 output;
     Table table;
  };

  struct Waveform {
     uint1 halt;
     uint1 writable;
    uint16 overflow;
     uint6 data[64];
     uint6 index;
  };

  uint1 enable;
  uint1 envelopes;  //0 = disable, 1 = enable
  uint2 masterVolume;
  Operator carrier;
  Modulator modulator;
  Waveform waveform;
};
