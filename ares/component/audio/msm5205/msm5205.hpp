#pragma once

namespace ares {

//OKI Semiconductor MSM5205

struct MSM5205 {
  //msm5205.cpp
  auto sample() const -> i12;
  auto scaler() const -> u32;

  auto setReset(n1 reset) -> void;
  auto setWidth(n1 width) -> void;
  auto setScaler(n2 frequency) -> void;
  auto setData(n4 data) -> void;

  auto clock() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1  reset;   //RESET
    n1  width;   //4B
    n2  scaler;  //S0-S1
    n4  data;    //D0-D7
    i12 sample;  //DAOUT
    i8  step;
  } io;

//unserialized:
  s32 lookup[49 * 16];
};

}
