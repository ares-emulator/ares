#pragma once

namespace ares {

//Texas Instruments SN76489

struct SN76489 {
  //sn76489.cpp
  auto clock() -> array<n4[4]>;
  auto write(n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Tone {
    //sn76489.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n4  volume = 15;
    n10 counter;
    n10 pitch;
    n1  output;
  };

  struct Noise {
    //sn76489.cpp
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n4  volume = 15;
    n10 counter;
    n10 pitch;
    n1  enable;
    n2  rate;
    n16 lfsr = 0x8000;
    n1  flip;
    n1  output;
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
