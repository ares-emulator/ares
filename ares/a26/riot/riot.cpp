#include <a26/a26.hpp>

namespace ares::Atari2600 {

RIOT riot;

#include "io.cpp"
#include "serialization.cpp"

auto RIOT::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RIOT");

  ram.allocate(128);
}

auto RIOT::unload() -> void {
  ram.reset();
  node.reset();
}

auto RIOT::main() -> void {
  if(--timer.counter == 0xff) {
    timer.interval = 1;
    timer.interruptFlag = 1;
  }

  step(timer.interval);
}

auto RIOT::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto RIOT::power(bool reset) -> void {
  Thread::create(system.frequency() / 3, {&RIOT::main, this});
  timer = {};
  port[0] = {};
  port[0].data = 0xff;
  port[1] = {};
  port[1].data = 0xff;
  leftDifficulty = 1;
  rightDifficulty = 1;
  tvType = 1;
}

}
