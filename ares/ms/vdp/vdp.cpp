#include <ms/ms.hpp>

namespace ares::MasterSystem {

VDP vdp;
#include "io.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(16_KiB);
  cram.allocate(!Model::GameGear() ? 32 : 64);

  node = parent->append<Node::Component>("VDP");

  screen = node->append<Node::Screen>("Screen");

  if(Model::MasterSystem()) {
    screen->colors(1 << 6, {&VDP::colorMasterSystem, this});
    screen->setSize(256, 240);
    screen->setScale(1.0, 1.0);
    screen->setAspect(8.0, 7.0);
  }

  if(Model::GameGear()) {
    screen->colors(1 << 12, {&VDP::colorGameGear, this});
    screen->setSize(160, 144);
    screen->setScale(1.0, 1.0);
    screen->setAspect(1.0, 1.0);

    interframeBlending = screen->append<Node::Boolean>("Interframe Blending", true, [&](auto value) {
      screen->setInterframeBlending(value);
    });
    interframeBlending->setDynamic(true);
  }

  debugger.load(node);
}

auto VDP::unload() -> void {
  vram.reset();
  cram.reset();
  node = {};
  screen = {};
  interframeBlending = {};
  debugger = {};
}

auto VDP::main() -> void {
  if(io.vcounter <= vlines()) {
    if(io.lcounter-- == 0) {
      io.lcounter = io.lineCounter;
      io.intLine = 1;
    }
  } else {
    io.lcounter = io.lineCounter;
  }

  if(io.vcounter == vlines() + 1) {
    io.intFrame = 1;
  }

  //684 clocks/scanline
  uint y = io.vcounter;
  sprite.setup(y);
  if(y < vlines()) {
    uint32* screen = buffer + (24 + y) * 256;
    for(uint x : range(256)) {
      background.run(x, y);
      sprite.run(x, y);
      step(2);

      uint12 color = palette(16 | io.backdropColor);
      if(!io.leftClip || x >= 8) {
        if(background.output.priority || !sprite.output.color) {
          color = palette(background.output.palette << 4 | background.output.color);
        } else if(sprite.output.color) {
          color = palette(16 | sprite.output.color);
        }
      }
      if(!io.displayEnable) color = 0;
      *screen++ = color;
    }
  } else {
    //Vblank
    step(512);
  }
  step(172);

  if(io.vcounter == 240) scheduler.exit(Event::Frame);
}

auto VDP::step(uint clocks) -> void {
  while(clocks--) {
    if(++io.hcounter == 684) {
      io.hcounter = 0;
      if(++io.vcounter == (Region::NTSC() ? 262 : 312)) {
        io.vcounter = 0;
      }
    }

    cpu.setIRQ((io.lineInterrupts && io.intLine) || (io.frameInterrupts && io.intFrame));
    Thread::step(1);
    Thread::synchronize(cpu);
  }
}

auto VDP::refresh() -> void {
  if(Model::MasterSystem()) {
    //center the video output vertically in the viewport
    uint32* centered = buffer;
    if(vlines() == 224) centered += 16 * 256;
    if(vlines() == 240) centered += 24 * 256;

    screen->refresh(centered, 256 * sizeof(uint32), 256, 240);
  }

  if(Model::GameGear()) {
    screen->refresh(buffer + 48 * 256 + 48, 256 * sizeof(uint32), 160, 144);
  }
}

auto VDP::vlines() -> uint {
  switch(io.mode) {
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

  memory::fill<uint32>(buffer, 256 * 264);
  for(auto& byte : vram) byte = 0x00;
  for(auto& byte : cram) byte = 0x00;
  io = {};

  background.power();
  sprite.power();
}

auto VDP::palette(uint5 index) -> uint12 {
  //Master System and Game Gear approximate TMS9918A colors by converting to RGB6 palette colors
  static uint6 palette[16] = {
    0x00, 0x00, 0x08, 0x0c, 0x10, 0x30, 0x01, 0x3c,
    0x02, 0x03, 0x05, 0x0f, 0x04, 0x33, 0x15, 0x3f,
  };
  if(!io.mode.bit(3)) return palette[index.bit(0,3)];
  if(Model::MasterSystem()) return cram[index];
  if(Model::GameGear()) return cram[index * 2 + 0] << 0 | cram[index * 2 + 1] << 8;
  return 0;
}

}
