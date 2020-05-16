#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RSP rsp;
#include "core/core.cpp"
#include "io.cpp"
#include "io-scc.cpp"
#include "serialization.cpp"

auto RSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RSP");

  dmem.allocate(4_KiB);
  imem.allocate(4_KiB);
}

auto RSP::unload() -> void {
  dmem.reset();
  imem.reset();
  node = {};
}

auto RSP::main() -> void {
  if(status.halted) return step(93'750'000 / 48'000);
  instruction();
  step(6);
}

auto RSP::step(uint clocks) -> void {
  clock += clocks;
}

auto RSP::power() -> void {
  Thread::reset();
  powerCore();
  dma = {};
  status = {};
}

}
