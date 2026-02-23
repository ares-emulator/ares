#pragma once

namespace ares {

//General Instrument AY-3-8910

struct AY38910 {
  virtual auto readIO(n1 port) -> n8 { return 0xff; }
  virtual auto writeIO(n1 port, n8 data) -> void {}

  //ay38910.cpp
  auto clock() -> array<n4[3]>;
  auto read() -> n8;
  auto write(n8 data) -> void;
  auto select(n4 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //ay38910.cpp
    auto clock() -> void;

    n12 counter;
    n12 period;
    n1  output;
  };

  struct Noise {
    //ay38910.cpp
    auto clock() -> void;

    n5  counter;
    n5  period;
    n1  flip;
    n17 lfsr = 1;
    n1  output;
  };

  struct Envelope {
    //ay38910.cpp
    auto clock() -> void;

    n16 counter;
    n16 period;
    n1  holding;
    n1  attacking;
    n1  hold;
    n1  alternate;
    n1  attack;
    n1  repeat;  //continue
    n4  output;
  };

  struct Channel {
    n1 tone;      //0 = enable, 1 = disable
    n1 noise;     //0 = enable, 1 = disable
    n1 envelope;  //0 = use fixed volume, 1 = use envelope phase
    n4 volume;
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
