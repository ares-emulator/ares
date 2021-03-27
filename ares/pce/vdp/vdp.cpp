#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.cpp"
#else
#include <pce/pce.hpp>

namespace ares::PCEngine {

VDP vdp;
#include "vce.cpp"
#include "vdc.cpp"
#include "vpc.cpp"
#include "irq.cpp"
#include "dma.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 1365, 263);
  screen->colors(1 << 10, {&VDP::color, this});
  screen->setSize(1088, 239);
  screen->setScale(0.25, 1.0);
  screen->setAspect(8.0, 7.0);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(1024, 239);
    if(value == 1) screen->setSize(1088, 239);
  });
  overscan->setDynamic(true);

  vce.debugger.load(vce, node);
  vdc0.debugger.load(vdc0, node); if(Model::SuperGrafx())
  vdc1.debugger.load(vdc1, node);
}

auto VDP::unload() -> void {
  vce.debugger = {};
  vdc0.debugger = {}; if(Model::SuperGrafx())
  vdc1.debugger = {};
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  vdc0.hsync(); if(Model::SuperGrafx())
  vdc1.hsync();

  if(io.vcounter == 0) {
    vdc0.vsync(); if(Model::SuperGrafx())
    vdc1.vsync();
  }

  auto output = screen->pixels().data() + 1365 * io.vcounter;

  while(io.hcounter <= 1360) {
    vdc0.hclock(); if(Model::SuperGrafx())
    vdc1.hclock();

    n10 color;
    if(Model::SuperGrafx() == 0) color = vdc0.bus();
    if(Model::SuperGrafx() == 1) color = vpc.bus(io.hcounter);
    color = vce.io.grayscale << 9 | vce.cram.read(color);

    switch(vce.clock()) {
    case 4: *output++ = color;
    case 3: *output++ = color;
    case 2: *output++ = color;
    case 1: *output++ = color;
    }

    step(vce.clock());
  }

  step(1365 - io.hcounter);
  vdc0.vclock(); if(Model::SuperGrafx())
  vdc1.vclock();

  io.hcounter = 0;
  if(++io.vcounter >= 262 + vce.io.extraLine) {
    io.vcounter = 0;
    if(overscan->value() == 0) screen->setViewport(96, 21, 1024, 239);
    if(overscan->value() == 1) screen->setViewport(96, 21, 1088, 239);
    screen->frame();
    scheduler.exit(Event::Frame);
  }
}

auto VDP::step(u32 clocks) -> void {
  io.hcounter += clocks;
  vdc0.dma.step(clocks); if(Model::SuperGrafx())
  vdc1.dma.step(clocks);

  Thread::step(clocks);
  synchronize(cpu);
}

auto VDP::power() -> void {
  Thread::create(system.colorburst() * 6.0, {&VDP::main, this});
  screen->power();

  vce.power();
  vdc0.power(); if(Model::SuperGrafx())
  vdc1.power(); if(Model::SuperGrafx())
  vpc.power();
}

}
#endif
