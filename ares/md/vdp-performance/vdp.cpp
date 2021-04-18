#include <md/md.hpp>

namespace ares::MegaDrive {

VDP vdp;
#include "psg.cpp"
#include "render.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "background.cpp"
#include "object.cpp"
#include "sprite.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 1280, 480);
  screen->colors(1 << 16, {&VDP::color, this});
  screen->setSize(1280, 480);
  screen->setScale(0.25, 0.5);
  screen->setAspect(1.0, 1.0);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(1280, 448);
    if(value == 1) screen->setSize(1280, 480);
  });
  overscan->setDynamic(true);

  psg.load(node);
  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger = {};
  psg.unload();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  if(state.vcounter < screenHeight()) {
    step(1280);
    if(!runAhead()) {
      render();
      m32x.vdp.scanline(pixels(), state.vcounter);
    }
    if(latch.horizontalInterruptCounter-- == 0) {
      latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
      if(io.horizontalBlankInterruptEnable) {
        cpu.raise(CPU::Interrupt::HorizontalBlank);
      }
    }
    cartridge.hblank(1);
    step(430);
  } else if(state.vcounter == screenHeight()) {
    if(io.verticalBlankInterruptEnable) {
      io.vblankIRQ = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }
    cartridge.vblank(1);
    apu.setINT(true);
    step(1286);
    cartridge.hblank(1);
    apu.setINT(false);
    step(424);
  } else {
    step(1280);
    cartridge.hblank(1);
    step(430);
  }

  cpu.lower(CPU::Interrupt::HorizontalBlank);
  cartridge.hblank(0);

  state.hdot = 0;
  state.hcounter = 0;
  state.vcounter++;

  if(state.vcounter == 240) {
    if(latch.interlace == 0) screen->setProgressive(1);
    if(latch.interlace == 1) screen->setInterlace(latch.field);
    screen->setViewport(0, 0, screen->width(), screen->height());
    screen->frame();
    scheduler.exit(Event::Frame);
  } else if(state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.field = state.field;
    latch.interlace = io.interlaceMode == 3;
    latch.overscan = io.overscan;
    latch.displayWidth = io.displayWidth;
    latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
    io.vblankIRQ = false;
    cpu.lower(CPU::Interrupt::VerticalBlank);
    cartridge.vblank(0);
  }
}

auto VDP::step(s32 clocks) -> void {
  state.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency() / 2.0, {&VDP::main, this});
  screen->power();

  for(auto& data : vram.pixels) data = 0;
  for(auto& data : vram.memory) data = 0;
  vram.size = 32768;
  vram.mode = 0;

  for(auto& data : vsram.memory) data = 0;

  for(auto& data : cram.memory) data = 0;

  psg.power(reset);
  dma.power();

  planeA.io = {};
  window.io = {};
  planeB.io = {};

  sprite.io = {};
  for(auto& object : sprite.oam) object = {};
  for(auto& object : sprite.objects) object = {};

  state = {};
  io = {};
  latch = {};
}

}
