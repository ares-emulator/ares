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
  timer.prescaler = interval;
  timer.interruptEnable = interruptEnable;
  timer.interruptFlag = 0;
  timer.underflow = 0;
  timer.holdZero = 0;
  timer.justWrapped = 0;
}

auto RIOT::clockTimer() -> void {
  timer.justWrapped = 0;

  if(timer.prescaler == 0) timer.prescaler = 1;
  if(--timer.prescaler) return;

  timer.prescaler = timer.interval;

  if(timer.holdZero) {
    timer.holdZero = 0;
    timer.underflow = 1;
    timer.counter = 0xff;
    timer.interval = 1;
    timer.prescaler = 1;
    timer.interruptFlag = 1;
    timer.justWrapped = 1;
    return;
  }

  if(timer.counter == 0x00) {
    timer.holdZero = 1;
    return;
  }

  timer.counter--;

  if(timer.underflow) {
    timer.interval = 1;
    timer.prescaler = 1;
  }
}

auto RIOT::main() -> void {
  clockTimer();
  step(1);
}

auto RIOT::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto RIOT::power(bool reset) -> void {
  Thread::create(system.frequency() / 3, std::bind_front(&RIOT::main, this));
  timer = {};
  timer.interval = 1;
  timer.prescaler = 1;
  timer.counter = 0x00;

  port[0] = {};
  port[0].data = 0xff;
  port[1] = {};
  port[1].data = 0xff;
  leftDifficulty = 1;
  leftDifficultyLatch = 0;
  rightDifficulty = 1;
  rightDifficultyLatch = 0;
  tvType = 1;
  tvTypeLatch = 0;
}
}
