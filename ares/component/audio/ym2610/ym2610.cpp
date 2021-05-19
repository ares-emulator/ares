#include <ares/ares.hpp>
#include "ym2610.hpp"

namespace ares {

#include "io.cpp"
#include "serialization.cpp"

auto YM2610::power() -> void {
  fm.power();
  ssg.power();
}

}
