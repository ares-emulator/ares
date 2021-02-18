#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Timer timer;
#include "io.cpp"
#include "serialization.cpp"

auto Timer::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Timer");
}

auto Timer::unload() -> void {
  node.reset();
}

auto Timer::step(u32 clocks) -> void {
  counter.dotclock += clocks;
  counter.divclock += clocks;

  {
    if(timers[0].clock == 0) {
      timers[0].step(clocks);
    }

    if(timers[1].clock == 0) {
      timers[1].step(clocks);
    }

    if(timers[2].divider == 0) {
      if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
        timers[2].step(clocks);
      }
    }
  }

  while(counter.dotclock >= 5) {
    counter.dotclock -= 5;
    if(timers[0].clock == 1) {
      timers[0].step();
    }
  }

  while(counter.divclock >= 8) {
    counter.divclock -= 8;
    if(timers[2].divider == 1) {
      if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
        timers[2].step();
      }
    }
  }
}

auto Timer::hsync(bool line) -> void {
  if(timers[0].synchronize)
  switch(timers[0].mode) {
  case 0: timers[0].paused = line == 1; break;
  case 1: if(line == 1) timers[0].counter = 0; break;
  case 2: if(line == 1) timers[0].counter = 0; timers[0].paused = line == 0; break;
  case 3: if(line == 1) timers[0].paused = timers[0].synchronize = 0; break;
  }

  if(timers[1].clock == 1 && line == 1) {
    timers[1].step();
  }
}

auto Timer::vsync(bool line) -> void {
  if(timers[1].synchronize)
  switch(timers[1].mode) {
  case 0: timers[1].paused = line == 1; break;
  case 1: if(line == 1) timers[1].counter = 0; break;
  case 2: if(line == 1) timers[1].counter = 0; timers[1].paused = line == 0; break;
  case 3: if(line == 1) timers[1].paused = timers[1].synchronize = 0; break;
  }
}

auto Timer::power(bool reset) -> void {
  Memory::Interface::setWaitStates(2, 2, 2);

  counter.dotclock = 0;
  counter.divclock = 0;
  for(auto& timer : timers) {
    timer.counter = 0;
    timer.target = 0;
    timer.synchronize = 0;
    timer.mode = 0;
    timer.resetMode = 0;
    timer.irqOnTarget = 0;
    timer.irqOnSaturate = 0;
    timer.irqRepeat = 0;
    timer.irqMode = 0;
    timer.clock = 0;
    timer.divider = 0;
    timer.irqLine = 1;
    timer.reachedTarget = 0;
    timer.reachedSaturate = 0;
    timer.unknown = 0;
    timer.paused = 0;
    timer.irqTriggered = 0;
  }
}

auto Timer::Source::step(u32 clocks) -> void {
  if(synchronize && paused) return;

  while(clocks--) {
    counter++;

    //counter value can be read in the range of 0..target (inclusive)
    if(u16(counter - 1) == target) {
      reachedTarget = 1;
      if(resetMode == 1) counter = 0;
      if(irqOnTarget) irq();
    }

    if(counter == 0xffff) {
      reachedSaturate = 1;
      if(resetMode == 0) counter = 0;
      if(irqOnSaturate) irq();
    }
  }
}

auto Timer::Source::irq() -> void {
  if(!irqTriggered) {
    if(irqMode == 0) {
      interrupt.pulse(Interrupt::Timer0 + id);
    } else {
      irqLine = !irqLine;
      if(!irqLine) interrupt.pulse(Interrupt::Timer0 + id);
    }
    if(!irqRepeat) irqTriggered = 1;
  }
}

auto Timer::Source::reset() -> void {
  counter = 0;

  switch(id) {
  case 0: paused = mode == 3; break;
  case 1: paused = mode == 3; break;
  case 2: paused = mode == 3 || mode == 0; break;
  }
}

}
