#include <ng/ng.hpp>

namespace ares::NeoGeo {

GPU gpu;
#include "color.cpp"
#include "render.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GPU");

  screen = node->append<Node::Video::Screen>("Screen", 320, 256);
  screen->colors(1 << 16, {&GPU::color, this});
  screen->setSize(304, 224);
  screen->setScale(2.0, 2.0);
  screen->setAspect(1.0, 1.0);

  vram.allocate(68_KiB >> 1);
  pram.allocate(16_KiB >> 1);

  debugger.load(node);
}

auto GPU::unload() -> void {
  debugger.unload(node);
  vram.reset();
  pram.reset();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto GPU::step(u32 clocks) -> void {
  if(io.timerCounter && !--io.timerCounter) {
    if(io.timerReloadOnZero) {
      io.timerCounter = io.timerReload;
    }
    if(irq.timerAcknowledge && io.timerInterruptEnable) {
      irq.timerAcknowledge = 0;
      cpu.raise(CPU::Interrupt::Timer);
    }
  }
  Thread::step(clocks);
  Thread::synchronize();
}

auto GPU::main() -> void {
  step(1);
  if(++io.hcounter == 384) {
    io.hcounter = 0;
    if(++io.vcounter == 264) {
      io.vcounter = 0;
      if(irq.vblankAcknowledge) {
        irq.vblankAcknowledge = 0;
        cpu.raise(CPU::Interrupt::Vblank);
      }
      if(io.timerReloadOnVblank) {
        io.timerCounter = io.timerReload;
      }
      frame();
    }
  }

  if(io.vcounter >= 8 && io.hcounter == 56) {
    render(io.vcounter - 8);
  }
}

auto GPU::frame() -> void {
  screen->setViewport(8, 16, 304, 224);
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto GPU::power(bool reset) -> void {
  Thread::create(6'000'000, {&GPU::main, this});
  screen->power();
  io = {};
  irq = {};
  cpu.raise(CPU::Interrupt::Power);
}

}
