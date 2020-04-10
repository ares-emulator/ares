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
  node = parent->append<Node::Component>("VDP");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(3 * (1 << 9), {&VDP::color, this});
  screen->setSize(1280, 480);
  screen->setScale(0.25, 0.50);
  screen->setAspect(1.0, 1.0);

  overscan = screen->append<Node::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(1280, 448);
    if(value == 1) screen->setSize(1280, 480);
  });
  overscan->setDynamic(true);

  debugger.load(node);
}

auto VDP::unload() -> void {
  node = {};
  screen = {};
  overscan = {};
  debugger = {};
}

auto VDP::main() -> void {
  scanline();

  cpu.lower(CPU::Interrupt::HorizontalBlank);
  apu.setINT(false);

  if(state.vcounter == 0) {
    latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
    io.vblankIRQ = false;
    cpu.lower(CPU::Interrupt::VerticalBlank);
  }

  if(state.vcounter == screenHeight()) {
    if(io.verticalBlankInterruptEnable) {
      io.vblankIRQ = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }
    //todo: should only stay high for ~2573/2 clocks
    apu.setINT(true);
  }

  if(state.vcounter < screenHeight()) {
    while(state.hcounter < 1280) {
      run();
      state.hdot++;
      step(pixelWidth());
    }

    if(latch.horizontalInterruptCounter-- == 0) {
      latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
      if(io.horizontalBlankInterruptEnable) {
        cpu.raise(CPU::Interrupt::HorizontalBlank);
      }
    }

    step(430);
  } else {
    step(1710);
  }

  state.hdot = 0;
  state.hcounter = 0;
  if(++state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.overscan = io.overscan;
  }
  latch.displayWidth = io.displayWidth;
}

auto VDP::step(uint clocks) -> void {
  state.hcounter += clocks;
  while(clocks--) {
    dma.run();
    Thread::step(1);
    Thread::synchronize(cpu, apu);
  }
}

auto VDP::refresh() -> void {
  auto data = output;

  if(overscan->value() == 0) {
    if(latch.overscan) data += 16 * 1280;
    screen->refresh(data, 1280 * sizeof(uint32), 1280, 448);
  }

  if(overscan->value() == 1) {
    if(!latch.overscan) data -= 16 * 1280;
    screen->refresh(data, 1280 * sizeof(uint32), 1280, 480);
  }
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency() / 2.0, {&VDP::main, this});

  output = buffer + 16 * 1280;  //overscan offset

  if(!reset) {
    for(auto& data : vram.memory) data = 0;
    for(auto& data : vsram.memory) data = 0;
    for(auto& data : cram.memory) data = 0;
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
