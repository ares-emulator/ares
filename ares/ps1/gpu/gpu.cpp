#include <ps1/ps1.hpp>

namespace ares::PlayStation {

GPU gpu;
#include "io.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("GPU");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1, [&](uint32 color) -> uint64 {
    return -1ll;
  });
  screen->setSize(320, 240);
}

auto GPU::unload() -> void {
  screen.reset();
  node.reset();
}

auto GPU::main() -> void {
  if(io.vcounter == 240) {
    cpu.interrupt.raise(CPU::Interrupt::Vblank);
  }

  if(++io.vcounter == 262) {
    io.vcounter = 0;
    cpu.interrupt.lower(CPU::Interrupt::Vblank);
    refreshed = true;
  }

  step(33'868'800 / 60 / 262);
}

auto GPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto GPU::refresh() -> void {
  memory::fill<u32>(output, 320 * 240);
  screen->refresh((uint32*)output, 320 * sizeof(uint32), 320, 240);
}

auto GPU::power(bool reset) -> void {
  Thread::reset();
  refreshed = false;
  io = {};
}

}
