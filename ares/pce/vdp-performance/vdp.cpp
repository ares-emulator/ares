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
  node = parent->append<Node::Component>("VDP");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1 << 10, {&VDP::color, this});
  screen->setSize(1088, 239);
  screen->setScale(0.25, 1.0);
  screen->setAspect(8.0, 7.0);

  vce.debugger.load(vce, parent);
  vdc0.debugger.load(vdc0, parent); if(Model::SuperGrafx())
  vdc1.debugger.load(vdc1, parent);
}

auto VDP::unload() -> void {
  node = {};
  screen = {};
  vce.debugger = {};
  vdc0.debugger = {}; if(Model::SuperGrafx())
  vdc1.debugger = {};
}

auto VDP::main() -> void {
  vdc0.hsync(); if(Model::SuperGrafx())
  vdc1.hsync();

  if(io.vcounter == 0) {
    vdc0.vsync(); if(Model::SuperGrafx())
    vdc1.vsync();
    width256 = 0, width344 = 0, width512 = 0;
  }

  step(512);

  vdc0.hclock(); if(Model::SuperGrafx())
  vdc1.hclock();

  if(io.vcounter >= 21 && io.vcounter < 239 + 21) {
    switch(vce.width()) {
    case 256: widths[io.vcounter] = 256; width256 = 1; break;
    case 344: widths[io.vcounter] = 344; width344 = 1; break;
    case 512: widths[io.vcounter] = 512; width512 = 1; break;
    }

    auto output = buffer + 1365 * io.vcounter;

    if(Model::SuperGrafx() == 0) {
      for(uint x : range(vce.width())) {
        output[x] = vce.io.grayscale << 9 | vce.cram.read(vdc0.output[x]);
      }
    }

    if(Model::SuperGrafx() == 1) {
      vpc.render();
      for(uint x : range(vce.width())) {
        output[x] = vce.io.grayscale << 9 | vce.cram.read(vpc.output[x]);
      }
    }
  }

  step(1365 - io.hcounter);

  vdc0.vclock(); if(Model::SuperGrafx())
  vdc1.vclock();

  io.hcounter = 0;
  if(++io.vcounter >= 262 + vce.io.extraLine) {
    io.vcounter = 0;
    scheduler.exit(Event::Frame);
  }
}

auto VDP::step(uint clocks) -> void {
  io.hcounter += clocks;
  vdc0.dma.step(clocks); if(Model::SuperGrafx())
  vdc1.dma.step(clocks);

  Thread::step(clocks);
  synchronize(cpu);
}

auto VDP::refresh() -> void {
  if(width256 == 1 && width344 == 0 && width512 == 0) {
    return screen->refresh(buffer + 1365 * 21, 1365 * sizeof(uint32), 256, 239);
  }

  if(width256 == 0 && width344 == 1 && width512 == 0) {
    return screen->refresh(buffer + 1365 * 21, 1365 * sizeof(uint32), 344, 239);
  }

  if(width256 == 0 && width344 == 0 && width512 == 1) {
    return screen->refresh(buffer + 1365 * 21, 1365 * sizeof(uint32), 512, 239);
  }

  //this frame contains mixed resolutions: normalize every scanline to 1024-width
  for(uint y = 21; y < 239 + 21; y++) {
    auto output = buffer + 1365 * y;
    switch(widths[y]) {

    case 256: {
      auto source = &output[256];
      auto target = &output[256 * 4];
      for(uint x : range(256)) {
        auto color = *--source;
        *--target = color;
        *--target = color;
        *--target = color;
        *--target = color;
      }
    } break;

    case 344: {
      auto source = &output[344];
      auto target = &output[344 * 3];
      for(uint x : range(344)) {
        auto color = *--source;
        *--target = color;
        *--target = color;
        *--target = color;
      }
    } break;

    case 512: {
      auto source = &output[512];
      auto target = &output[512 * 2];
      for(uint x : range(512)) {
        auto color = *--source;
        *--target = color;
        *--target = color;
      }
    } break;

    }
  }
  return screen->refresh(buffer + 1365 * 21, 1365 * sizeof(uint32), 1024, 239);
}

auto VDP::power() -> void {
  Thread::create(system.colorburst() * 6.0, {&VDP::main, this});

  vce.power();
  vdc0.power(); if(Model::SuperGrafx())
  vdc1.power(); if(Model::SuperGrafx())
  vpc.power();
}

}
