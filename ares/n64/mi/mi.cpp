#include <n64/n64.hpp>

namespace ares::Nintendo64 {

MI mi;
#include "io.cpp"
#include "serialization.cpp"

auto MI::pollInterrupts() -> void {
  bool line = 0;
  line |= irq.sp.line & irq.sp.mask;
  line |= irq.si.line & irq.si.mask;
  line |= irq.ai.line & irq.ai.mask;
  line |= irq.vi.line & irq.vi.mask;
  line |= irq.pi.line & irq.pi.mask;
  line |= irq.dp.line & irq.dp.mask;
  cpu.scc.cause.interruptPending.bit(2) = line;
}

auto MI::power() -> void {
  irq.sp.line = 0;
  irq.si.line = 0;
  irq.ai.line = 0;
  irq.vi.line = 0;
  irq.pi.line = 0;
  irq.dp.line = 0;
}

}
