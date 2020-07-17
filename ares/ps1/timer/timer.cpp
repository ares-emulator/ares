#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Timer timer;
#include "io.cpp"
#include "serialization.cpp"

auto Timer::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Timer");
}

auto Timer::unload() -> void {
  node.reset();
}

auto Timer::step(uint clocks) -> void {
  counter.dotclock += clocks;
  counter.hclock   += clocks;
  counter.divclock += clocks;

  {
    if(timers[0].clock == 0) {
      timers[0].step(clocks);
    }

    if(timers[1].clock == 0) {
      timers[1].step(clocks);
    }

    if(timers[2].divider == 0 && timers[2].disable == 0) {
      timers[2].step(clocks);
    }
  }

  while(counter.dotclock >= 8) {
    counter.dotclock -= 8;
    if(timers[0].clock == 1) {
      timers[0].step();
    }
  }

  while(counter.hclock >= 2154) {
    counter.hclock -= 2154;
    if(timers[1].clock == 1) {
      timers[1].step();
    }
  }

  while(counter.divclock >= 8) {
    counter.divclock -= 8;
    if(timers[2].divider == 1 && timers[2].disable == 0) {
      timers[2].step();
    }
  }
}

auto Timer::poll() -> void {
  interrupt.drive(Interrupt::Timer0, timers[0].irqLine == 0);
  interrupt.drive(Interrupt::Timer1, timers[1].irqLine == 0);
  interrupt.drive(Interrupt::Timer2, timers[2].irqLine == 0);
}

auto Timer::power(bool reset) -> void {
}

auto Timer::Source::step(uint clocks) -> void {
  if(counter < target && counter + clocks >= target) {
    reachedTarget = 1;
    if(irqOnTarget) irq();
  }

  if(counter + clocks >= 0x10000) {
    reachedSaturate = 1;
    if(irqOnSaturate) irq();
  }

  if(resetMode == 0 && counter + clocks >= target) {
    counter = counter + clocks - target;
  } else if(resetMode == 1 && counter + clocks >= 0x10000) {
    counter = counter + clocks - 0x10000;
  } else {
    counter = counter + clocks;
  }
}

auto Timer::Source::irq() -> void {
  if(irqMode == 0) {
    irqLine = 0;
  } else {
    irqLine = !irqLine;
  }
  self.poll();

  if(!irqRepeat) {
    irqOnTarget = 0;
    irqOnSaturate = 0;
  }
}

}
