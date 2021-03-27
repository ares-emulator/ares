#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.cpp"
#else
#include <md/md.hpp>

namespace ares::MegaDrive {

VDP vdp;
#include "memory.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "render.cpp"
#include "background.cpp"
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

  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger.unload();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  scanline();

  cpu.lower(CPU::Interrupt::HorizontalBlank);
  if(Mega32X()) m32x.hblank(0);

  if(state.vcounter == 0) {
    latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
    io.vblankIRQ = false;
    cpu.lower(CPU::Interrupt::VerticalBlank);
    if(Mega32X()) m32x.vblank(0);
  }

  if(state.vcounter < screenHeight()) {
    while(state.hcounter < 1280) {
      run();
      state.hdot++;
      step(pixelWidth());
    }
    if(Mega32X()) m32x.vdp.scanline(pixels(), state.vcounter);

    if(latch.horizontalInterruptCounter-- == 0) {
      latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
      if(io.horizontalBlankInterruptEnable) {
        cpu.raise(CPU::Interrupt::HorizontalBlank);
      }
    }
    if(Mega32X()) m32x.hblank(1);

    step(430);
  } else if(state.vcounter == screenHeight()) {
    if(io.verticalBlankInterruptEnable) {
      io.vblankIRQ = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }

    if(Mega32X()) m32x.vblank(1);
    apu.setINT(true);
    step(1286);
    if(Mega32X()) m32x.hblank(1);
    apu.setINT(false);
    step(424);
  } else {
    step(1280);
    if(Mega32X()) m32x.hblank(1);
    step(430);
  }

  state.hdot = 0;
  state.hcounter = 0;
  if(++state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.field = state.field;
    latch.interlace = io.interlaceMode == 3;
    latch.overscan = io.overscan;
  }
  latch.displayWidth = io.displayWidth;
}

auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;
  while(clocks--) {
    dma.run();
    Thread::step(1);
    Thread::synchronize(cpu, apu);
  }
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency() / 2.0, {&VDP::main, this});
  screen->power();

  if(!reset) {
    for(auto& data : vram.memory ) data = 0;
    for(auto& data : vsram.memory) data = 0;
    for(auto& data : cram.memory ) data = 0;
  }

  vram.mode = 0;
  io = {};
  latch = {};
  state = {};

  planeA.power();
  window.power();
  planeB.power();
  sprite.power();
  dma.power();
}

}
#endif
