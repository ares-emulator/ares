#pragma once

namespace ares {

//OKI Semiconductor MSM5205

struct MSM5205 {
  //msm5205.cpp
  auto sample() const -> int12;
  auto scaler() const -> uint;

  auto setReset(uint1 reset) -> void;
  auto setWidth(uint1 width) -> void;
  auto setScaler(uint2 frequency) -> void;
  auto setData(uint4 data) -> void;

  auto clock() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint1 reset;   //RESET
    uint1 width;   //4B
    uint2 scaler;  //S0-S1
    uint4 data;    //D0-D7
    int12 sample;  //DAOUT
     int8 step;
  } io;

//unserialized:
  int lookup[49 * 16];
};

}
