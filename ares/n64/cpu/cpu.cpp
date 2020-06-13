#include <n64/n64.hpp>

namespace ares::Nintendo64 {

CPU cpu;
#include "core/core.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  instruction();
}

auto CPU::step(uint clocks) -> void {
   vi.clock -= clocks;
   ai.clock -= clocks;
  rsp.clock -= clocks;
  rdp.clock -= clocks;
  while( vi.clock < 0) vi.main();
  while( ai.clock < 0) ai.main();
  while(rsp.clock < 0) rsp.main();
  while(rdp.clock < 0) rdp.main();

  if(scc.count < scc.compare && scc.count + clocks >= scc.compare) {
    scc.cause.interruptPending.bit(Interrupt::Timer) = 1;
  }
  scc.count += clocks;
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  powerR4300(reset);
}

}
