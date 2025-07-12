#include <ms/ms.hpp>

namespace ares::MasterSystem {

VDP vdp;
#include "io.cpp"
#include "irq.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(16_KiB);
  cram.allocate(32);

  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 284, screenHeight());

  u32 defaultRevision = 2;
  if((Model::MarkIII() || Model::MasterSystemI()) && Region::NTSCJ()) defaultRevision = 1;
  revision = node->append<Node::Setting::Natural>("Revision", defaultRevision);
  revision->setAllowedValues({1, 2});
  screen->refreshRateHint(system.colorburst() * 15, 3420, Region::PAL() ? 313 : 262);

  if(Display::CRT()) {
    screen->colors(1 << 6, {&VDP::colorMasterSystem, this});
    screen->setSize(284, screenHeight());
    screen->setScale(1.0, 1.0);
    Region::PAL() ? screen->setAspect(19.0, 14.0) : screen->setAspect(8.0, 7.0);

    colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
      screen->resetPalette();
    });
    colorEmulation->setDynamic(true);
  }

  if(Display::LCD()) {
    screen->colors(1 << 12, {&VDP::colorGameGear, this});
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
  debugger = {};
  interframeBlending.reset();
  revision.reset();
  screen->quit();
  node->remove(screen);
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
  if(io.vcounter < vlines()) {
    u8 y = io.vcounter;
    background.setup(y);
    sprite.setup(y);
    for(u8 x : range(256)) {
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
  Thread::create(system.colorburst() * 15.0 / 5.0, {&VDP::main, this});
  screen->power();

  for(auto& byte : vram) byte = 0x00;
  for(auto& byte : cram) byte = 0x00;

  background.power();
  sprite.power();
  dac.power();
  irq.power();
  io = {};
  latch = {};
}

}
