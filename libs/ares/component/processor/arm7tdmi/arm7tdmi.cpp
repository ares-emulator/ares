#include <ares/ares.hpp>
#include "arm7tdmi.hpp"

namespace ares {

#include "registers.cpp"
#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions-arm.cpp"
#include "instructions-thumb.cpp"
#include "coprocessor.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

ARM7TDMI::ARM7TDMI() {
  armInitialize();
  thumbInitialize();
}

auto ARM7TDMI::power() -> void {
  processor = {};
  processor.r15.modify = [&] { processor.r15.data &= ~1; pipeline.reload = true; };
  processor.rNULL.modify = [&] { processor.rNULL.data = 0; };
  processor.spsrNULL.readonly = true;
  pipeline = {};
  carry = 0;
  irq = 0;
  cpsr().f = 1;
  exception(PSR::SVC, 0x00);
}

}
