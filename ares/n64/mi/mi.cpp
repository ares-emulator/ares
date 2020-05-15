#include <n64/n64.hpp>

namespace ares::Nintendo64 {

MI mi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto MI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("MI");

  debugger.load(node);
}

auto MI::unload() -> void {
  node.reset();
  debugger = {};
}

auto MI::raise(IRQ source) -> void {
  if(debugger.tracer.interrupt->enabled()) {
    if(!irq.sp.line && source == IRQ::SP) debugger.interrupt("SP");
    if(!irq.si.line && source == IRQ::SI) debugger.interrupt("SI");
    if(!irq.ai.line && source == IRQ::AI) debugger.interrupt("AI");
    if(!irq.vi.line && source == IRQ::VI) debugger.interrupt("VI");
    if(!irq.pi.line && source == IRQ::PI) debugger.interrupt("PI");
    if(!irq.dp.line && source == IRQ::DP) debugger.interrupt("DP");
  }

  switch(source) {
  case IRQ::SP: irq.sp.line = 1; break;
  case IRQ::SI: irq.si.line = 1; break;
  case IRQ::AI: irq.ai.line = 1; break;
  case IRQ::VI: irq.vi.line = 1; break;
  case IRQ::PI: irq.pi.line = 1; break;
  case IRQ::DP: irq.dp.line = 1; break;
  }
  poll();
}

auto MI::lower(IRQ source) -> void {
  switch(source) {
  case IRQ::SP: irq.sp.line = 0; break;
  case IRQ::SI: irq.si.line = 0; break;
  case IRQ::AI: irq.ai.line = 0; break;
  case IRQ::VI: irq.vi.line = 0; break;
  case IRQ::PI: irq.pi.line = 0; break;
  case IRQ::DP: irq.dp.line = 0; break;
  }
  poll();
}

auto MI::poll() -> void {
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
  irq.sp.line = 0; irq.sp.mask = 1;
  irq.si.line = 0; irq.si.mask = 1;
  irq.ai.line = 0; irq.ai.mask = 1;
  irq.vi.line = 0; irq.vi.mask = 1;
  irq.pi.line = 0; irq.pi.mask = 1;
  irq.dp.line = 0; irq.dp.mask = 1;
}

}
