#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "core/core.cpp"
#include "dma.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
  ram.allocate(2_MiB);
  cache.allocate(1_KiB);
}

auto CPU::unload() -> void {
  ram.reset();
  cache.reset();
  node.reset();
}

auto CPU::main() -> void {
  instruction();
}

auto CPU::step(uint clocks) -> void {
  gpu.clock -= clocks;
  spu.clock -= clocks;
  while(gpu.clock < 0) gpu.main();
  while(spu.clock < 0) spu.main();
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  powerCore(reset);
  ram.fill();
  cache.fill();
  dma.power(reset);
}

}
