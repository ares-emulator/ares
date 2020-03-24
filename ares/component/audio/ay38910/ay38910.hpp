#pragma once

namespace ares {

//General Instrument AY-3-8910

struct AY38910 {
  virtual auto readIO(uint1 port) -> uint8 { return 0xff; }
  virtual auto writeIO(uint1 port, uint8 data) -> void {}

  //ay38910.cpp
  auto clock() -> array<uint4[3]>;
  auto read() -> uint8;
  auto write(uint8 data) -> void;
  auto select(uint4 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //ay38910.cpp
    auto clock() -> void;

    uint12 counter;
    uint12 period;
     uint1 output;
  };

  struct Noise {
    //ay38910.cpp
    auto clock() -> void;

     uint5 counter;
     uint5 period;
     uint1 flip;
    uint17 lfsr = 1;
     uint1 output;
  };

  struct Envelope {
    //ay38910.cpp
    auto clock() -> void;

    uint16 counter;
    uint16 period;
     uint1 holding;
     uint1 attacking;
     uint1 hold;
     uint1 alternate;
     uint1 attack;
     uint1 repeat;  //continue
     uint4 output;
  };

  struct Channel {
    uint1 tone;      //0 = enable, 1 = disable
    uint1 noise;     //0 = enable, 1 = disable
    uint1 envelope;  //0 = use fixed volume, 1 = use envelope phase
    uint4 volume;
  };

  struct Port {
    uint1 direction;  //0 = input, 1 = output
    uint8 data;
  };

  struct IO {
    uint4 register;
  };

  Tone toneA;
  Tone toneB;
  Tone toneC;
  Noise noise;
  Envelope envelope;
  Channel channelA;
  Channel channelB;
  Channel channelC;
  Port portA;
  Port portB;
  IO io;
};

}
