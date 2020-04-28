#pragma once

//NEC VR4300

namespace ares {

struct VR4300 {
  //vr4300.cpp
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

}
