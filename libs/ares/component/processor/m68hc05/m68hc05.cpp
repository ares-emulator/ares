#include <ares/ares.hpp>
#include "m68hc05.hpp"

namespace ares {

#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto M68HC05::power() -> void {
  A   = 0;
  X   = 0;
  PC  = 0;
  SP  = 0xff;
  CCR = 0;
}

}
