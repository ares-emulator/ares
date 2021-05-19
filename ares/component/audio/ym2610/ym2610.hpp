#pragma once

#include <component/audio/ym2612/ym2612.hpp>
#include <component/audio/ym2149/ym2149.hpp>

namespace ares {

//Yamaha YM2610 (OPNB)

struct YM2610 {
  //ym2610.cpp
  auto power() -> void;

  //io.cpp
  auto read(n2 address) -> n8;
  auto write(n2 address, n8 data) -> void;
  auto writeLower(n8 data) -> void;
  auto writeUpper(n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  n9 register;
  YM2612 fm;
  YM2149 ssg;
};

}
