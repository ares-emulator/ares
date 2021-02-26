#pragma once

namespace ares {

//Yamaha YM2610 (OPNB)

struct YM2610 {
  //ym2610.cpp
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

}
