#include <gba/gba.hpp>

//The only PPU state the CPU needs on every cycle is raised IRQs and DMAs,
//which occur independently of the render process.
//Display exists to put these events on a separate thread,
//so the CPU and PPU can run out-of-order.

//hdraw:    1006 cycles
//hblank:    226 cycles
//scanline: 1232 cycles

//vdraw:     160 scanlines (197120 cycles)
//vblank:     68 scanlines ( 83776 cycles)
//frame:     228 scanlines (280896 cycles)

namespace ares::GameBoyAdvance {

Display display;
#include "io.cpp"
#include "serialization.cpp"

auto Display::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Display");
}

auto Display::unload() -> void {
  node.reset();
}

auto Display::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto Display::main() -> void {
  cpu.keypad.run();

  io.vblank = io.vcounter >= 160 && io.vcounter <= 226;

  step(1);

  io.vcoincidence = io.vcounter == io.vcompare;

  if(io.vcounter == 160) {
    if(io.irqvblank) cpu.setInterruptFlag(CPU::Interrupt::VBlank);
  }

  step(1);

  if(io.irqvcoincidence) {
    if(io.vcoincidence) cpu.setInterruptFlag(CPU::Interrupt::VCoincidence);
  }

  if(io.vcounter == 160) {
    cpu.dmaVblank();
  }

  step(3);

  if(io.vcounter == 162) {
    if(videoCapture) cpu.dmac.channel[3].enable = 0;
    videoCapture = !videoCapture && cpu.dmac.channel[3].timingMode == 3 && cpu.dmac.channel[3].enable;
  }
  if(io.vcounter >= 2 && io.vcounter < 162 && videoCapture) cpu.dmaHDMA();

  step(1002);

  io.hblank = 1;

  step(1);
  if(io.irqhblank) cpu.setInterruptFlag(CPU::Interrupt::HBlank);

  step(1);
  if(io.vcounter < 160) cpu.dmaHblank();

  step(223);
  io.hblank = 0;
  if(++io.vcounter == 228) io.vcounter = 0;
}

auto Display::power() -> void {
  Thread::create(system.frequency(), {&Display::main, this});

  for(u32 n = 0x004; n <= 0x007; n++) bus.io[n] = this;

  io = {};
}

}
