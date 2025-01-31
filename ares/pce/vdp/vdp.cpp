#include <pce/pce.hpp>

namespace ares::PCEngine {

VDPBase vdp;
VDP vdpImpl;

#define vdp vdpImpl

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

auto VDPBase::setAccurate(bool value) -> void {
  accurate = value;
  if(value) {
    implementation = &vdpImpl;
    vce = &vdpImpl.vce;
    vdc0 = &vdpImpl.vdc0;
    vdc1 = &vdpImpl.vdc1;
    vpc = &vdpImpl.vpc;
  } else {
    implementation = &vdpPerformanceImpl;
    vce = &vdpPerformanceImpl.vce;
    vdc0 = &vdpPerformanceImpl.vdc0;
    vdc1 = &vdpPerformanceImpl.vdc1;
    vpc = &vdpPerformanceImpl.vpc;
  }
}

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 1365, 263);
  screen->colors(1 << 10, {&VDP::color, this});
  screen->setSize(1128, 263);
  screen->setScale(0.25, 1.0);
  screen->setAspect(8.0, 7.0);
  screen->refreshRateHint(60); // TODO: More accurate refresh rate hint

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  vce.debugger.load(vce, node);
  vdc0.debugger.load(vdc0, node); if(Model::SuperGrafx())
  vdc1.debugger.load(vdc1, node);
}

auto VDP::unload() -> void {
  vce.debugger = {};
  vdc0.debugger = {}; if(Model::SuperGrafx())
  vdc1.debugger = {};
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

template<bool supergrafx> auto VDP::main() -> void {
  vdc0.hsync(); if(supergrafx)
  vdc1.hsync();

  if(io.vcounter == 0) {
    vdc0.vsync(); if(supergrafx)
    vdc1.vsync();
  }

  auto output = screen->pixels().data() + 1365 * io.vcounter;

  while(io.hcounter <= 1360) {
    vdc0.hclock(); if(supergrafx)
    vdc1.hclock();

    n10 color;
    if(!supergrafx) color = vdc0.bus();
    if( supergrafx) color = vpc.bus(io.hcounter);
    color = vce.io.grayscale << 9 | vce.cram.read(color);

    switch(vce.clock()) {
    case 4: *output++ = color; [[fallthrough]];
    case 3: *output++ = color; [[fallthrough]];
    case 2: *output++ = color; [[fallthrough]];
    case 1: *output++ = color;
    }

    step<supergrafx>(vce.clock());
  }

  step<supergrafx>(1365 - io.hcounter);
  vdc0.vclock(); if(Model::SuperGrafx())
  vdc1.vclock();

  io.hcounter = 0;
  if(++io.vcounter >= 262 + vce.io.extraLine) {
    io.vcounter = 0;
    screen->setViewport(48, 16, screen->width(), 242);
    screen->frame();
    scheduler.exit(Event::Frame);
  }
}

template<bool supergrafx> auto VDP::step(u32 clocks) -> void {
  io.hcounter += clocks;
  vdc0.dma.step(clocks); if(supergrafx)
  vdc1.dma.step(clocks);

  Thread::step(clocks);
  synchronize(cpu);
}

auto VDP::power() -> void {
  if(Model::SuperGrafx()) Thread::create(system.colorburst() * 6.0, {&VDP::main<true>, this});
  else                    Thread::create(system.colorburst() * 6.0, {&VDP::main<false>, this});

  screen->power();

  vce.power();
  vdc0.power(); if(Model::SuperGrafx())
  vdc1.power(); if(Model::SuperGrafx())
  vpc.power();
}

}
