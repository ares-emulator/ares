#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Interrupt interrupt;
#include "io.cpp"
#include "serialization.cpp"

auto Interrupt::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Interrupt");
}

auto Interrupt::unload() -> void {
  node.reset();
}

auto Interrupt::poll() -> void {
  line = 0;
  line |= vblank.stat & vblank.mask;
  line |= gpu.stat & gpu.mask;
  line |= cdrom.stat & cdrom.mask;
  line |= dma.stat & dma.mask;
  line |= timer0.stat & timer0.mask;
  line |= timer1.stat & timer1.mask;
  line |= timer2.stat & timer2.mask;
  line |= peripheral.stat & peripheral.mask;
  line |= sio.stat & sio.mask;
  line |= spu.stat & spu.mask;
  line |= pio.stat & pio.mask;
  cpu.scc.cause.interruptPending.bit(2) = line;
}

auto Interrupt::raise(uint source) -> void {
  if(source ==  0 && !vblank.line) return vblank.line = vblank.stat = 1, poll();
  if(source ==  1 && !gpu.line) return gpu.line = gpu.stat = 1, poll();
  if(source ==  2 && !cdrom.line) return cdrom.line = cdrom.stat = 1, poll();
  if(source ==  3 && !dma.line) return dma.line = dma.stat = 1, poll();
  if(source ==  4 && !timer0.line) return timer0.line = timer0.stat = 1, poll();
  if(source ==  5 && !timer1.line) return timer1.line = timer1.stat = 1, poll();
  if(source ==  6 && !timer2.line) return timer2.line = timer2.stat = 1, poll();
  if(source ==  7 && !peripheral.line) return peripheral.line = peripheral.stat = 1, poll();
  if(source ==  8 && !sio.line) return sio.line = sio.stat = 1, poll();
  if(source ==  9 && !spu.line) return spu.line = spu.stat = 1, poll();
  if(source == 10 && !pio.line) return pio.line = pio.stat = 1, poll();
}

auto Interrupt::lower(uint source) -> void {
  if(source ==  0 && vblank.line) return vblank.line = 0, poll();
  if(source ==  1 && gpu.line) return gpu.line = 0, poll();
  if(source ==  2 && cdrom.line) return cdrom.line = 0, poll();
  if(source ==  3 && dma.line) return dma.line = 0, poll();
  if(source ==  4 && timer0.line) return timer0.line = 0, poll();
  if(source ==  5 && timer1.line) return timer1.line = 0, poll();
  if(source ==  6 && timer2.line) return timer2.line = 0, poll();
  if(source ==  7 && peripheral.line) return peripheral.line = 0, poll();
  if(source ==  8 && sio.line) return sio.line = 0, poll();
  if(source ==  9 && spu.line) return spu.line = 0, poll();
  if(source == 10 && pio.line) return pio.line = 0, poll();
}

auto Interrupt::pulse(uint source) -> void {
  raise(source);
  lower(source);
}

auto Interrupt::drive(uint source, bool line) -> void {
  if(line == 0) lower(source);
  if(line == 1) raise(source);
}

auto Interrupt::power(bool reset) -> void {
}

}
