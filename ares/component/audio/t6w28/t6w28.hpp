#pragma once

namespace ares {

//T6W28 (SN76489 variant)

struct T6W28 {
  virtual auto writePitch(u32) -> void = 0;

  //t6w28.cpp
  auto clock() -> array<n4[8]>;
  auto writeLeft(n8 data) -> void;
  auto writeRight(n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //t6w28.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 counter;
    n10 pitch;
    n1  output;
    struct Volume {
      n4 left  = 15;
      n4 right = 15;
    } volume;
  };

  struct Noise {
    //t6w28.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n10 counter;
    n10 pitch;
    n1  enable;
    n2  rate;
    n15 lfsr = 0x4000;
    n1  flip;
    n1  output;
    struct Volume {
      n4 left  = 15;
      n4 right = 15;
    } volume;
  };

  struct Latch {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 type;
    n2 channel;
  };

  Tone  tone0;
  Tone  tone1;
  Tone  tone2;
  Noise noise;
  Latch latch;
};

}
