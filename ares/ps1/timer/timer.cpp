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
  if(!htimer.synchronizeEnable) {
    htimer.step(clocks);
  } else {
  }

  if(!vtimer.synchronizeEnable) {
    vtimer.step(clocks);
  } else {
  }

  if(!ftimer.synchronizeEnable) {
    ftimer.step(clocks);
  } else {
    if(ftimer.synchronizeMode == 1) ftimer.step(clocks);
    if(ftimer.synchronizeMode == 2) ftimer.step(clocks);
  }
}

auto Timer::poll() -> void {
  interrupt.drive(Interrupt::Timer0, htimer.irqLine == 0);
  interrupt.drive(Interrupt::Timer1, vtimer.irqLine == 0);
  interrupt.drive(Interrupt::Timer2, ftimer.irqLine == 0);
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
