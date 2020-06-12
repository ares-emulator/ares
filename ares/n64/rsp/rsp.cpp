#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RSP rsp;
#include "core/core.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RSP");
  dmem.allocate(4_KiB);
  imem.allocate(4_KiB);
  debugger.load(node);
}

auto RSP::unload() -> void {
  dmem.reset();
  imem.reset();
  node = {};
  debugger = {};
}

auto RSP::main() -> void {
  if(status.halted) return step(96);
  instruction();
}

auto RSP::step(uint clocks) -> void {
  clock += clocks;
}

auto RSP::power() -> void {
  Thread::reset();
  powerCore();
  dma = {};
  status.semaphore = 0;
  status.halted = 1;
  status.broken = 0;
  status.full = 0;
  status.singleStep = 0;
  status.interruptOnBreak = 0;
  for(auto& signal : status.signal) signal = 0;
}

}
