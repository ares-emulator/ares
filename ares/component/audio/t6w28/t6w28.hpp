#pragma once

namespace ares {

//T6W28 (SN76489 variant)

struct T6W28 {
  //t6w28.cpp
  auto clock() -> array<uint4[8]>;
  auto writeLeft(uint8 data) -> void;
  auto writeRight(uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //t6w28.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint10 counter;
    uint10 pitch;
    uint1  output;
    struct Volume {
      uint4 left  = 15;
      uint4 right = 15;
    } volume;
  };

  struct Noise {
    //t6w28.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint10 counter;
    uint10 pitch;
    uint1  enable;
    uint2  rate;
    uint15 lfsr = 0x4000;
    uint1  flip;
    uint1  output;
    struct Volume {
      uint4 left  = 15;
      uint4 right = 15;
    } volume;
  };

  struct IO {
    uint3 register;
  };

  Tone tone0;
  Tone tone1;
  Tone tone2;
  Noise noise;
  IO io;
};

}
