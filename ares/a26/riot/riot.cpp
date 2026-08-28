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

auto RIOT::reloadTimer(n8 data, n16 interval, n1 interruptEnable) -> void {
  timer.counter = data;
  timer.interval = interval;
  timer.prescaler = 1;
  timer.interruptEnable = interruptEnable;
  timer.interruptFlag = 0;
  timer.justWrapped = 0;
}

auto RIOT::clockTimer() -> void {
  timer.justWrapped = 0;

  if(--timer.prescaler) return;

  if(timer.interruptFlag) {
    timer.counter--;
    timer.prescaler = 1;
    return;
  }

  timer.prescaler = timer.interval;

  if(timer.counter == 0x00) {
    timer.counter = 0xff;
    timer.prescaler = 1;
    timer.interruptFlag = 1;
    timer.justWrapped = 1;
    return;
  }

  timer.counter--;
}

auto RIOT::main() -> void {
  clockTimer();
  step(1);
}

auto RIOT::step(u32 clocks) -> void {
  Thread::step(clocks * 3);
  Thread::synchronize();
}

auto RIOT::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&RIOT::main, this));
  timer = {};
  //R6532 reset timer state is unspecified. Stella uses random/1024; MAME starts 256/1024.
  //Use MAME's deterministic compatibility phase.
  timer.interval = 1024;
  timer.prescaler = 1024;
  timer.counter = 0xff;

  port[0] = {};
  port[1] = {};
  pa7 = {};
  pa7.level = 1;
  drivePortA();
  leftDifficulty = 1;
  leftDifficultyLatch = 0;
  rightDifficulty = 1;
  rightDifficultyLatch = 0;
  tvType = 1;
  tvTypeLatch = 0;
}
}
