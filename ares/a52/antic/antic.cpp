#include <a52/a52.hpp>

namespace ares::Atari5200 {

ANTIC antic;

#include "io.cpp"
#include "timing.cpp"
#include "display-list.cpp"
#include "dma.cpp"
#include "playfield.cpp"
#include "interrupt.cpp"
#include "serialization.cpp"

auto ANTIC::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("ANTIC");
}

auto ANTIC::unload() -> void {
  Thread::destroy();
  node = {};
}

auto ANTIC::main() -> void {
  scanline();
}

auto ANTIC::power() -> void {
  Thread::create(system.frequency(), std::bind_front(&ANTIC::main, this));
  io = {};
  counter = {};
  displayList.power();
  dma.power();
  playfield.power();
  wsync.power();
  interrupt.power();
  cpu.rdyLine(1);
  cpu.nmiLine(0);
}

}
