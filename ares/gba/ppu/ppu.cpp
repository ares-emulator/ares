#include <gba/gba.hpp>

//pixel:       4 cycles

//hdraw:      46 cycle wait period, then 240 pixels (total: 1006 cycles)
//hblank:    226 cycles
//scanline: 1232 cycles

//vdraw:     160 scanlines (197120 cycles)
//vblank:     68 scanlines ( 83776 cycles)
//frame:     228 scanlines (280896 cycles)

namespace ares::GameBoyAdvance {

PPU ppu;
#include "background.cpp"
#include "object.cpp"
#include "window.cpp"
#include "dac.cpp"
#include "io.cpp"
#include "memory.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  vram.allocate(96_KiB);
  pram.allocate(512);

  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 240, 160);
  screen->colors(1 << 15, {&PPU::color, this});
  screen->setSize(240, 160);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->setViewport(0, 0, 240, 160);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);

  rotation = screen->append<Node::Setting::String>("Orientation", "0°", [&](auto value) {
    if(value ==   "0°") screen->setRotation(  0);
    if(value ==  "90°") screen->setRotation( 90);
    if(value == "180°") screen->setRotation(180);
    if(value == "270°") screen->setRotation(270);
  });
  rotation->setDynamic(true);
  rotation->setAllowedValues({"0°", "90°", "180°", "270°"});

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger = {};
  colorEmulation.reset();
  interframeBlending.reset();
  rotation.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
  vram.reset();
  pram.reset();
}

inline auto PPU::blank() -> bool {
  return io.forceBlank || cpu.stopped();
}

auto PPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PPU::main() -> void {
  cpu.keypad.run();

  io.vblank = io.vcounter >= 160 && io.vcounter <= 226;
  io.vcoincidence = io.vcounter == io.vcompare;

  if(io.vcounter == 0) {
    frame();

    bg2.io.lx = bg2.io.x;
    bg2.io.ly = bg2.io.y;

    bg3.io.lx = bg3.io.x;
    bg3.io.ly = bg3.io.y;
  }

  if(io.vcounter == 160) {
    if(io.irqvblank) cpu.irq.flag |= CPU::Interrupt::VBlank;
    cpu.dmaVblank();
  }

  if(io.irqvcoincidence) {
    if(io.vcoincidence) cpu.irq.flag |= CPU::Interrupt::VCoincidence;
  }

  step(46);

  if(io.vcounter < 160) {
    u32 y = io.vcounter;
    bg0.scanline(y);
    bg1.scanline(y);
    bg2.scanline(y);
    bg3.scanline(y);
    objects.scanline(y);
    auto line = screen->pixels().data() + y * 240;
    for(u32 x : range(240)) {
      bg0.run(x, y);
      bg1.run(x, y);
      bg2.run(x, y);
      bg3.run(x, y);
      objects.run(x, y);
      window0.run(x, y);
      window1.run(x, y);
      window2.output = objects.output.window;
      window3.output = true;
      n15 color = dac.run(x, y);
      line[x] = color;
      step(4);
    }
  } else {
    step(960);
  }

  io.hblank = 1;
  if(io.irqhblank) cpu.irq.flag |= CPU::Interrupt::HBlank;
  if(io.vcounter < 160) cpu.dmaHblank();

  step(226);
  io.hblank = 0;
  if(++io.vcounter == 228) io.vcounter = 0;
  if(io.vcounter > 1 && io.vcounter < 162) cpu.dmaHDMA();
}

auto PPU::frame() -> void {
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::power() -> void {
  Thread::create(system.frequency(), {&PPU::main, this});
  screen->power();

  for(u32 n = 0x000; n <= 0x055; n++) bus.io[n] = this;

  for(u32 n = 0; n < 96 * 1024; n++) vram[n] = 0x00;
  for(u32 n = 0; n < 1024; n += 2) writePRAM(n, Half, 0x0000);
  for(u32 n = 0; n < 1024; n += 2) writeOAM(n, Half, 0x0000);

  io = {};
  for(auto& object : this->object) object = {};
  for(auto& param : this->objectParam) param = {};

  bg0.power(BG0);
  bg1.power(BG1);
  bg2.power(BG2);
  bg3.power(BG3);
  objects.power();
  window0.power(IN0);
  window1.power(IN1);
  window2.power(IN2);
  window3.power(OUT);
  dac.power();
}

}
