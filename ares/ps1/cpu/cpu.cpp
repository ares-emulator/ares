#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "core/core.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
  ram.allocate(2_MiB);
  cache.allocate(1_KiB);
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger = {};
  cache.reset();
  ram.reset();
  node.reset();
}

auto CPU::main() -> void {
  instruction();
}

auto CPU::step(uint clocks) -> void {
  dma.step(clocks);
  timer.step(clocks);

  gpu.clock -= clocks;
  spu.clock -= clocks;
  disc.clock -= clocks;
  peripheral.clock -= clocks;

  while(gpu.clock < 0) gpu.main();
  while(spu.clock < 0) spu.main();
  while(disc.clock < 0) disc.main();
  while(peripheral.clock < 0) peripheral.main();
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  powerCore(reset);
  ram.fill();
  cache.fill();
}

}
