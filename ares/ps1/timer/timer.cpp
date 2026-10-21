#include <ps1/ps1.hpp>

namespace ares::PlayStation {

constexpr u8 WAIT_CYCLES = 1;

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
  u32 timerClocks[3] = {clocks, clocks, clocks};

  for(u32 id : range(3)) {
    if(timers[id].wait == 0) continue;

    if(timerClocks[id] <= timers[id].wait) {
      timers[id].wait -= timerClocks[id];
      timerClocks[id] = 0;
    } else {
      timerClocks[id] -= timers[id].wait;
      timers[id].wait = 0;
    }
  }

  counter.divclock += timerClocks[2];

  {
    if(timers[0].clock == 0) {
      timers[0].step(timerClocks[0]);
    }

    if(timers[1].clock == 0) {
      timers[1].step(timerClocks[1]);
    }

    if(timers[2].divider == 0) {
      if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
        timers[2].step(timerClocks[2]);
      }
    }
  }

  u32 dividedClocks = counter.divclock / 8;
  counter.divclock %= 8;
  if(dividedClocks && timers[2].divider == 1) {
    if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
      timers[2].step(dividedClocks);
    }
  }

  updateNextEvent();
}

auto Timer::hsync(bool line) -> void {
  if(timers[0].syncActive())
  switch(timers[0].mode) {
  case 0: timers[0].paused = line == 1; break;
  case 1: if(line == 1) timers[0].counter = 0; break;
  case 2: if(line == 1) timers[0].counter = 0; timers[0].paused = line == 0; break;
  case 3: if(line == 1) { timers[0].paused = 0; timers[0].syncFreeRun = 1; } break;
  }

  if(timers[1].clock == 1 && line == 1) {
    timers[1].step();
  }

  updateNextEvent();
}

auto Timer::vsync(bool line) -> void {
  if(timers[1].syncActive())
  switch(timers[1].mode) {
  case 0: timers[1].paused = line == 1; break;
  case 1: if(line == 1) timers[1].counter = 0; break;
  case 2: if(line == 1) timers[1].counter = 0; timers[1].paused = line == 0; break;
  case 3: if(line == 1) { timers[1].paused = 0; timers[1].syncFreeRun = 1; } break;
  }

  updateNextEvent();
}

auto Timer::updateNextEvent() -> void {
  if(timers[0].clock == 0) {
    if(u32 clocks = timers[0].nextEvent()) cpu.setNextEvent(clocks);
  }

  if(timers[1].clock == 0) {
    if(u32 clocks = timers[1].nextEvent()) cpu.setNextEvent(clocks);
  }

  if(timers[2].synchronize == 0 || timers[2].mode == 1 || timers[2].mode == 2) {
    if(timers[2].divider == 0) {
      if(u32 clocks = timers[2].nextEvent()) cpu.setNextEvent(clocks);
    } else {
      if(u32 ticks = timers[2].nextEvent()) {
        u32 clocks = 8 - counter.divclock + (ticks - 1) * 8;
        cpu.setNextEvent(clocks);
      }
    }
  }
}

auto Timer::power(bool reset) -> void {
  counter.divclock = 0;
  for(auto& timer : timers) {
    timer.counter = 0;
    timer.target = 0;
    timer.synchronize = 0;
    timer.syncFreeRun = 0;
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
    timer.resetPending = 0;
    timer.wait = 0;
  }

  updateNextEvent();
}

auto Timer::Source::step(u32 clocks) -> void {
  if(syncActive() && paused) return;

  if(!resetMode && !resetPending && !wait && !irqOnTarget && !irqOnSaturate) {
    u32 toTarget = (target - counter) & 0xffff;
    u32 toSaturate = (0xffff - counter) & 0xffff;
    if(!toTarget) toTarget = 0x10000;
    if(!toSaturate) toSaturate = 0x10000;
    if(clocks >= toTarget) reachedTarget = 1;
    if(clocks >= toSaturate) reachedSaturate = 1;
    counter += clocks;
    return;
  }

  while(clocks > 0) {
    if(resetPending) {
      counter = 0;
      resetPending = 0;
      wait = 1;
      clocks--;
      continue;
    }

    if(wait) {
      if(clocks <= wait) {
        wait -= clocks;
        break;
      }

      clocks -= wait;
      wait = 0;
      continue;
    }

    u16 counter16 = u16(counter);

    u32 toTarget   = ((target - counter16) & 0xffff);
    u32 toSaturate = ((0xffff - counter16) & 0xffff);
    if(toTarget   == 0) toTarget   = 0x10000;
    if(toSaturate == 0) toSaturate = 0x10000;

    u32 stepClocks = max<u32>(1, min(clocks, toTarget, toSaturate));

    counter += stepClocks;
    clocks -= stepClocks;

    u16 current = u16(counter);
    bool hitTarget   = current == target;
    bool hitSaturate = current == 0xffff;

    if(hitTarget) {
      reachedTarget = 1;
      if(irqOnTarget) irq();
    }

    if(hitSaturate) {
      reachedSaturate = 1;
      if(irqOnSaturate) irq();
    }

    if(hitTarget && resetMode == 1) {
      resetPending = 1;
    }
  }
}

auto Timer::Source::nextEvent() const -> u32 {
  if(syncActive() && paused) return 0;

  bool targetIrq   = irqOnTarget   && (irqRepeat || !irqTriggered);
  bool saturateIrq = irqOnSaturate && (irqRepeat || !irqTriggered);
  if(!targetIrq && !saturateIrq) return 0;

  if(resetPending) return 1;

  u16 counter16 = u16(counter);
  u32 toTarget   = ((target - counter16) & 0xffff);
  u32 toSaturate = ((0xffff - counter16) & 0xffff);
  if(toTarget   == 0) toTarget   = 0x10000;
  if(toSaturate == 0) toSaturate = 0x10000;

  u32 next = 0x10000;
  if(targetIrq)   next = min(next, toTarget);
  if(saturateIrq) next = min(next, toSaturate);

  return wait + next;
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
  resetPending = 0;
  wait = 0;

  switch(id) {
  case 0:
    if(!syncActive()) paused = 0;
    else if(mode == 0) paused = gpu.hblank();
    else if(mode == 1) paused = 0;
    else if(mode == 2) paused = !gpu.hblank();
    else if(mode == 3) paused = 1;
    break;
  case 1:
    if(!syncActive()) paused = 0;
    else if(mode == 0) paused = gpu.vblank();
    else if(mode == 1) paused = 0;
    else if(mode == 2) paused = !gpu.vblank();
    else if(mode == 3) paused = 1;
    break;
  case 2: paused = mode == 3 || mode == 0; break;
  }
}

}
