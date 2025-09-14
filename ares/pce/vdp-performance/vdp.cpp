#include <pce/pce.hpp>

#define VCE VCEPerformance
#define VDC VDCPerformance
#define VDP VDPPerformance
#define VPC VPCPerformance
#define vdp vdpPerformanceImpl

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

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  screen->colors(1 << 10, std::bind_front(&VDP::color, this));
  screen->setSize(1128, 263);
  screen->setScale(0.25, 1.0);
  screen->setAspect(8.0, 7.0);
  screen->refreshRateHint(60); // TODO: More accurate refresh rate hint

  vce.debugger.load(vce, parent);
  vdc0.debugger.load(vdc0, parent); if(Model::SuperGrafx())
  vdc1.debugger.load(vdc1, parent);
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

auto VDP::main() -> void {
  vdc0.hsync(); if(Model::SuperGrafx())
  vdc1.hsync();

  if(io.vcounter == 0) {
    vdc0.vsync(); if(Model::SuperGrafx())
    vdc1.vsync();
  }

  step(renderingCycle);

  vdc0.hclock(); if(Model::SuperGrafx())
  vdc1.hclock();

  if(io.vcounter >= 21 && io.vcounter < 239 + 21) {
    auto line = screen->pixels().data() + 1365 * io.vcounter + 48;
    auto clock = vce.clock();

    if(Model::SuperGrafx() == 0) {
      for(u32 x : range(vce.width())) {
        u32 color = vce.io.grayscale << 9 | vce.cram.read(vdc0.output[x]);
        switch(clock) {
        case 4: *line++ = color; [[fallthrough]];
        case 3: *line++ = color; [[fallthrough]];
        case 2: *line++ = color; [[fallthrough]];
        case 1: *line++ = color;
        }
      }
    }

    if(Model::SuperGrafx() == 1) {
      vpc.render();
      for(u32 x : range(vce.width())) {
        u32 color = vce.io.grayscale << 9 | vce.cram.read(vpc.output[x]);
        switch(clock) {
        case 4: *line++ = color; [[fallthrough]];
        case 3: *line++ = color; [[fallthrough]];
        case 2: *line++ = color; [[fallthrough]];
        case 1: *line++ = color;
        }
      }
    }
  }

  step(1365 - io.hcounter);

  vdc0.vclock(); if(Model::SuperGrafx())
  vdc1.vclock();

  io.hcounter = 0;
  if(++io.vcounter >= 262 + vce.io.extraLine) {
    io.vcounter = 0;
    screen->setViewport(0, 16, screen->width(), 242);
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
  Thread::create(system.colorburst() * 6.0, std::bind_front(&VDP::main, this));
  screen->power();

  vce.power();
  vdc0.power(); if(Model::SuperGrafx())
  vdc1.power(); if(Model::SuperGrafx())
  vpc.power();

  renderingCycle = 512;
  string game = system.game();
  if(game == "Kore ga Pro Yakyuu '89") renderingCycle = 128;
  if(game == "Kore ga Pro Yakyuu '90") renderingCycle = 128;
  if(game == "TV Sports Baseball") renderingCycle = 128;
  if(game == "TV Sports Football") renderingCycle = 128;
  if(game == "TV Sports Hockey") renderingCycle = 128;
  if(game == "Valkyrie no Densetsu") renderingCycle = 128;
  if(game == "Victory Run") renderingCycle = 32;
}

}
