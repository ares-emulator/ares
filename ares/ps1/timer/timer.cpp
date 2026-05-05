#include <ps1/ps1.hpp>

namespace ares::PlayStation {

constexpr u8 WAIT_CYCLES = 2;

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
  counter.divclock += clocks;

  {
    if(timers[0].clock == 0) {
      timers[0].step(clocks - timers[0].wait);
    }

    if(timers[1].clock == 0) {
      timers[1].step(clocks - timers[1].wait);
    }

    if(timers[2].divider == 0) {
      if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
        timers[2].step(clocks - timers[2].wait);
      }
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

  for(auto& timer : timers) {
    if(timer.wait > 0) timer.wait = max(0, (i32)timer.wait - (i32)clocks);
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
  if((synchronize && paused) || wait) return;

  while(clocks > 0) {
    u16 counter16 = u16(counter);

    u32 toTarget   = ((target + 1 - counter16) & 0xffff);
    u32 toSaturate = ((0x10000 - counter16) & 0xffff);

    u32 stepClocks = std::max<u32>(1, std::min(clocks, std::min(toTarget, toSaturate)));

    counter += stepClocks;
    clocks -= stepClocks;

    //counter value can be read in the range of 0..target (inclusive)
    u16 last = u16(counter - 1);

    if(last == target) {
      reachedTarget = 1;
      if(resetMode == 1) {
        wait = WAIT_CYCLES;
        counter = 0;
      }
      if(irqOnTarget) irq();
    }

    if(last == 0xffff) {
      reachedSaturate = 1;
      wait = 1;
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
      if(!irqLine) interrupt.raise(Interrupt::Timer0 + id);
      if( irqLine) interrupt.lower(Interrupt::Timer0 + id);
    }
    if(!irqRepeat) irqTriggered = 1;
  }
}

auto Timer::Source::reset() -> void {
  counter = 0;
  wait = 0;

  switch(id) {
  case 0: paused = mode == 3; break;
  case 1: paused = mode == 3; break;
  case 2: paused = mode == 3 || mode == 0; break;
  }
}

}
