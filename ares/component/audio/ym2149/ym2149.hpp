#pragma once

namespace ares {

//Yamaha YM2149 (General Instrument AY-3-8910 derivative)

struct YM2149 {
  //ym2149.cpp
  auto clock() -> array<uint5[3]>;
  auto read() -> uint8;
  auto write(uint8 data) -> void;
  auto select(uint4 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //ym2149.cpp
    auto clock() -> void;

    uint12 counter;
    uint12 period;
     uint4 unused;
     uint1 output;
  };

  struct Noise {
    //ym2149.cpp
    auto clock() -> void;

     uint5 counter;
     uint5 period;
     uint3 unused;
     uint1 flip;
    uint17 lfsr = 1;
     uint1 output;
  };

  struct Envelope {
    //ym2149.cpp
    auto clock() -> void;

    uint16 counter;
    uint16 period;
     uint1 holding;
     uint1 attacking;
     uint1 hold;
     uint1 alternate;
     uint1 attack;
     uint1 repeat;  //continue
     uint4 unused;
     uint5 output;
  };

  struct Channel {
    uint1 tone;      //0 = enable, 1 = disable
    uint1 noise;     //0 = enable, 1 = disable
    uint1 envelope;  //0 = use fixed volume, 1 = use envelope volume
    uint4 volume;
    uint3 unused;
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
