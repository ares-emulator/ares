#pragma once

namespace ares {

//Yamaha YM2149 (General Instrument AY-3-8910 derivative)

struct YM2149 {
  //ym2149.cpp
  auto clock() -> array<n5[3]>;
  auto read() -> n8;
  auto write(n8 data) -> void;
  auto select(n4 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //ym2149.cpp
    auto clock() -> void;

    n12 counter;
    n12 period;
    n4  unused;
    n1  output;
  };

  struct Noise {
    //ym2149.cpp
    auto clock() -> void;

    n5  counter;
    n5  period;
    n3  unused;
    n1  flip;
    n17 lfsr = 1;
    n1  output;
  };

  struct Envelope {
    //ym2149.cpp
    auto clock() -> void;

    n16 counter;
    n16 period;
    n1  holding;
    n1  attacking;
    n1  hold;
    n1  alternate;
    n1  attack;
    n1  repeat;  //continue
    n4  unused;
    n5  output;
  };

  struct Channel {
    n1 tone;      //0 = enable, 1 = disable
    n1 noise;     //0 = enable, 1 = disable
    n1 envelope;  //0 = use fixed volume, 1 = use envelope volume
    n4 volume;
    n3 unused;
  };

  struct Port {
    n1 direction;  //0 = input, 1 = output
    n8 data;
  };

  struct IO {
    n4 register;
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
