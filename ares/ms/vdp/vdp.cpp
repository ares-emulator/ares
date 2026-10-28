#include <ms/ms.hpp>

namespace ares::MasterSystem {

VDP vdp;
VDP vdp2;
#include "io.cpp"
#include "irq.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(Model::SystemE() ? 32_KiB : 16_KiB);
  cram.allocate(32);

  node = parent->append<Node::Object>(backLayer ? "Back VDP" : "VDP");

  if(backLayer) {
    revision = node->append<Node::Setting::Natural>("Revision", 1);
    revision->setAllowedValues({1});
    debugger.load(node);
    return;
  }

  screen = node->append<Node::Video::Screen>("Screen", 284, screenHeight());

  u32 defaultRevision = 2;
  if(((Model::MarkIII() || Model::MasterSystemI()) && Region::NTSCJ()) || Model::SystemE()) defaultRevision = 1;
  revision = node->append<Node::Setting::Natural>("Revision", defaultRevision);
  if(Model::SystemE()) revision->setAllowedValues({1});
  else revision->setAllowedValues({1, 2});
  screen->refreshRateHint(system.colorburst() * 15, 3420, Region::PAL() ? 313 : 262);

  if(Display::CRT()) {
    colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
      screen->resetPalette();
    });
    colorEmulation->setDynamic(true);

    screen->colors(1 << 6, std::bind_front(&VDP::colorMasterSystem, this));
    screen->setSize(284, screenHeight());
    screen->setScale(1.0, 1.0);
    Region::PAL() ? screen->setAspect(19.0, 14.0) : screen->setAspect(8.0, 7.0);
  }

  if(Display::LCD()) {
    screen->colors(1 << 12, std::bind_front(&VDP::colorGameGear, this));
    screen->setSize(160, 144);
    screen->setScale(1.0, 1.0);
    screen->setAspect(6.0, 5.0);
    screen->setViewport(0, 0, 284, screenHeight());

    interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
      screen->setInterframeBlending(value);
    });
    interframeBlending->setDynamic(true);
  }

  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger.memory = {};
  debugger.tracer = {};
  interframeBlending.reset();
  revision.reset();
  if(screen) screen->quit();
  if(screen) node->remove(screen);
  screen.reset();
  node.reset();
  vram.reset();
  cram.reset();
}

auto VDP::main() -> void {
  if(io.vcounter <= vlines()) {
    if(irq.line.counter-- == 0) {
      irq.line.counter = irq.line.coincidence;
      irq.line.pending = 1;
      irq.poll();
    }
  } else {
    irq.line.counter = irq.line.coincidence;
  }

  if(io.vcounter == vlines() + 1) {
    irq.frame.pending = 1;
    irq.poll();
  }

  //684 clocks/scanline
  dac.setup(io.vcounter);
  if(Model::SystemE()) {
    vdp2.io.vcounter = io.vcounter;
    vdp2.io.hcounter = io.hcounter;
    if(vdp2.io.vcounter <= vdp2.vlines()) {
      if(vdp2.irq.line.counter-- == 0) {
        vdp2.irq.line.counter = vdp2.irq.line.coincidence;
        vdp2.irq.line.pending = 1;
      }
    } else {
      vdp2.irq.line.counter = vdp2.irq.line.coincidence;
    }
    if(vdp2.io.vcounter == vdp2.vlines() + 1) vdp2.irq.frame.pending = 1;
    vdp2.dac.setup(io.vcounter);
  }
  if(io.vcounter < vlines()) {
    u8 y = io.vcounter;
    background.setup(y);
    sprite.setup(y);
    if(Model::SystemE()) {
      vdp2.background.setup(y);
      vdp2.sprite.setup(y);
    }
    for(u8 x : range(256)) {
      if(Model::SystemE()) {
        vdp2.background.run(x, y);
        vdp2.sprite.run(x, y);
        vdp2.dac.run(x, y);
      }
      background.run(x, y);
      sprite.run(x, y);
      dac.run(x, y);
      step(2);
    }
  } else {
    //Vblank
    step(512);
  }
  step(172);

  if(io.vcounter == 240) {
    if(Display::CRT()) {
      if(screen->overscan()) {
        screen->setSize(284, screenHeight());
        screen->setViewport(0, 0, 284, screenHeight());
      } else {
        int x = 13;
        int y = 27;
        int width = 284 - 28;
        int height = screenHeight() - 51;

        if(Region::PAL()) {
          y += 21;
          height -= 13;
        }

        screen->setSize(width, height);
        screen->setViewport(x, y, width, height);
      }
    } else if (Mode::MasterSystem()) {
      screen->setViewport(21, 27, 248, 200);
    }
    if(Mode::GameGear()) {
      screen->setViewport(61, 51, 160, 144);
    }
    screen->frame();
    scheduler.exit(Event::Frame);
  }

  if(io.vcounter < (Region::PAL() ? 311 : 260)) {
    io.ccounter++;  //C-sync counter
  }
}

auto VDP::step(u32 clocks) -> void {
  while(clocks--) {
    if(++io.hcounter == 684) {
      io.hcounter = 0;
      if(++io.vcounter == (Region::PAL() ? 313 : 262)) {
        io.vcounter = 0;
      }
    }

    irq.poll();
    Thread::step(1);
    Thread::synchronize(cpu);
  }
}

auto VDP::vlines() -> u32 {
  if(revision->value() == 1) return 192;

  switch(videoMode()) {
  default:     return 192;
  case 0b1011: return 224;
  case 0b1110: return 240;
  }
}

auto VDP::vblank() -> bool {
  return io.vcounter >= vlines();
}

auto VDP::power() -> void {
  if(!backLayer) {
    Thread::create(system.colorburst() * 15.0 / 5.0, std::bind_front(&VDP::main, this));
    screen->power();
  }

  for(auto& byte : vram) byte = 0x00;
  for(auto& byte : cram) byte = 0x00;

  background.power();
  sprite.power();
  dac.power();
  irq.power();
  io = {};
  latch = {};
  vramBank = 0;
}

}
