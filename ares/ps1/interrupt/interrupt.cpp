#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Interrupt interrupt;
#include "io.cpp"
#include "serialization.cpp"

auto Interrupt::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Interrupt");
}

auto Interrupt::unload() -> void {
  node.reset();
}

auto Interrupt::poll() -> void {
  bool interruptsWerePending = cpu.exception.interruptsPending();
  bool line = 0;
  line |= vblank.poll();
  line |= gpu.poll();
  line |= cdrom.poll();
  line |= dma.poll();
  line |= timer0.poll();
  line |= timer1.poll();
  line |= timer2.poll();
  line |= peripheral.poll();
  line |= sio.poll();
  line |= spu.poll();
  line |= pio.poll();
  cpu.scc.cause.interruptPending.bit(2) = line;
  if(!interruptsWerePending && cpu.exception.interruptsPending()) cpu.delay.interrupt = 2;
}

auto Interrupt::level(u32 source) -> bool {
  if(source ==  0) return vblank.level();
  if(source ==  1) return gpu.level();
  if(source ==  2) return cdrom.level();
  if(source ==  3) return dma.level();
  if(source ==  4) return timer0.level();
  if(source ==  5) return timer1.level();
  if(source ==  6) return timer2.level();
  if(source ==  7) return peripheral.level();
  if(source ==  8) return sio.level();
  if(source ==  9) return spu.level();
  if(source == 10) return pio.level();
  return 0;
}

auto Interrupt::raise(u32 source) -> void {
  if(source ==  0 && !vblank.line) {
    vblank.raise();
    poll();
  }
  if(source ==  1 && !gpu.line) {
    gpu.raise();
    poll();
  }
  if(source ==  2 && !cdrom.line) {
    cdrom.raise();
    poll();
  }
  if(source ==  3 && !dma.line) {
    dma.raise();
    poll();
  }
  if(source ==  4 && !timer0.line) {
    timer0.raise();
    poll();
  }
  if(source ==  5 && !timer1.line) {
    timer1.raise();
    poll();
  }
  if(source ==  6 && !timer2.line) {
    timer2.raise();
    poll();
  }
  if(source ==  7 && !peripheral.line) {
    peripheral.raise();
    poll();
  }
  if(source ==  8 && !sio.line) {
    sio.raise();
    poll();
  }
  if(source ==  9 && !spu.line) {
    spu.raise();
    poll();
  }
  if(source == 10 && !pio.line) {
    pio.raise();
    poll();
  }
}

auto Interrupt::lower(u32 source) -> void {
  if(source ==  0 && vblank.line) return vblank.lower(), poll();
  if(source ==  1 && gpu.line) return gpu.lower(), poll();
  if(source ==  2 && cdrom.line) return cdrom.lower(), poll();
  if(source ==  3 && dma.line) return dma.lower(), poll();
  if(source ==  4 && timer0.line) return timer0.lower(), poll();
  if(source ==  5 && timer1.line) return timer1.lower(), poll();
  if(source ==  6 && timer2.line) return timer2.lower(), poll();
  if(source ==  7 && peripheral.line) return peripheral.lower(), poll();
  if(source ==  8 && sio.line) return sio.lower(), poll();
  if(source ==  9 && spu.line) return spu.lower(), poll();
  if(source == 10 && pio.line) return pio.lower(), poll();
}

auto Interrupt::pulse(u32 source) -> void {
  raise(source);
  lower(source);
}

auto Interrupt::drive(u32 source, bool line) -> void {
  if(line == 0) lower(source);
  if(line == 1) raise(source);
}

auto Interrupt::power(bool reset) -> void {
  Memory::Interface::setWaitStates(2, 3, 2);
  vblank = {};
  gpu = {};
  cdrom = {};
  dma = {};
  timer0 = {};
  timer1 = {};
  timer2 = {};
  peripheral = {};
  sio = {};
  spu = {};
  pio = {};
}

}
