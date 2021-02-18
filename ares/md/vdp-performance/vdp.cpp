#include <md/md.hpp>

namespace ares::MegaDrive {

VDP vdp;
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

  screen = node->append<Node::Video::Screen>("Screen", 320, 480);
  screen->colors(3 * (1 << 9), {&VDP::color, this});
  screen->setSize(320, 480);
  screen->setScale(1.0, 0.5);
  screen->setAspect(1.0, 1.0);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(320, 448);
    if(value == 1) screen->setSize(320, 480);
  });
  overscan->setDynamic(true);

  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger = {};
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  //H = 0
  cpu.lower(CPU::Interrupt::HorizontalBlank);
  apu.setINT(false);

  if(state.vcounter == 0) {
    latch.displayWidth = io.displayWidth;
    latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
    io.vblankIRQ = false;
    cpu.lower(CPU::Interrupt::VerticalBlank);
  }

  if(state.vcounter == screenHeight()) {
    if(io.verticalBlankInterruptEnable) {
      io.vblankIRQ = true;
      cpu.raise(CPU::Interrupt::VerticalBlank);
    }
    apu.setINT(true);
  }

  step(512);
  //H = 512
  if(state.vcounter < screenHeight() && !runAhead()) {
    render();
  }

  step(768);
  //H = 1280
  if(state.vcounter < screenHeight()) {
    if(latch.horizontalInterruptCounter-- == 0) {
      latch.horizontalInterruptCounter = io.horizontalInterruptCounter;
      if(io.horizontalBlankInterruptEnable) {
        cpu.raise(CPU::Interrupt::HorizontalBlank);
      }
    }
  }

  step(430);
  //H = 0
  state.hdot = 0;
  state.hcounter = 0;
  state.vcounter++;

  if(state.vcounter == 240) {
    if(latch.interlace == 0) screen->setProgressive(1);
    if(latch.interlace == 1) screen->setInterlace(latch.field);
    screen->setViewport(0, 0, screen->width(), screen->height());
    screen->frame();
    scheduler.exit(Event::Frame);
  }

  if(state.vcounter >= frameHeight()) {
    state.vcounter = 0;
    state.field ^= 1;
    latch.field = state.field;
    latch.interlace = io.interlaceMode == 3;
    latch.overscan = io.overscan;
  }
}

auto VDP::step(u32 clocks) -> void {
  state.hcounter += clocks;

  if(!dma.io.enable || dma.io.wait) {
    dma.active = 0;
    Thread::step(clocks);
    Thread::synchronize(cpu, apu);
  } else while(clocks--) {
    dma.run();
    Thread::step(1);
    Thread::synchronize(cpu, apu);
  }
}

auto VDP::render() -> void {
  if(!io.displayEnable) return;

  if(state.vcounter < window.io.verticalOffset ^ window.io.verticalDirection) {
    window.renderWindow(0, screenWidth());
  } else if(!window.io.horizontalDirection) {
    window.renderWindow(0, window.io.horizontalOffset);
    planeA.renderScreen(window.io.horizontalOffset, screenWidth());
  } else {
    planeA.renderScreen(0, window.io.horizontalOffset);
    window.renderWindow(window.io.horizontalOffset, screenWidth());
  }
  planeB.renderScreen(0, screenWidth());
  sprite.render();

  u32* output = nullptr;
  if(overscan->value() == 0 && io.overscan == 0) {
    if(state.vcounter >= 224) return;
    output = screen->pixels().data() + (state.vcounter - 0) * 2 * 320;
  }
  if(overscan->value() == 0 && io.overscan == 1) {
    if(state.vcounter <=   7) return;
    if(state.vcounter >= 232) return;
    output = screen->pixels().data() + (state.vcounter - 8) * 2 * 320;
  }
  if(overscan->value() == 1 && io.overscan == 0) {
    if(state.vcounter >= 232) return;
    output = screen->pixels().data() + (state.vcounter + 8) * 2 * 320;
  }
  if(overscan->value() == 1 && io.overscan == 1) {
    output = screen->pixels().data() + (state.vcounter + 0) * 2 * 320;
  }
  if(latch.interlace) output += state.field * 320;

  auto A = &planeA.pixels[0];
  auto B = &planeB.pixels[0];
  auto S = &sprite.pixels[0];
  n7 c[4] = {0, 0, 0, io.backgroundColor};
  if(!io.shadowHighlightEnable) {
    auto p = &cram.palette[1 << 7];
    for(u32 x : range(screenWidth())) {
      c[0] = *A++;
      c[1] = *B++;
      c[2] = *S++;
      u32 l = lookupFG[c[0] >> 2 << 10 | c[1] >> 2 << 5 | c[2] >> 2];
      *output++ = p[c[l]];
    }
  } else {
    auto p = &cram.palette[0 << 7];
    for(u32 x : range(screenWidth())) {
      c[0] = *A++;
      c[1] = *B++;
      c[2] = *S++;
      u32 l = lookupFG[c[0] >> 2 << 10 | c[1] >> 2 << 5 | c[2] >> 2];
      u32 mode = (c[0] | c[1]) >> 2 & 1;  //0 = shadow, 1 = normal, 2 = highlight
      if(l == 2) {
        if(c[2] >= 0x70) {
          if(c[2] <= 0x72) mode = 1;
          else if(c[2] == 0x73) l = lookupBG[c[0] >> 2 << 5 | c[1] >> 2], mode++;
          else if(c[2] == 0x7b) l = lookupBG[c[0] >> 2 << 5 | c[1] >> 2], mode = 0;
          else mode |= c[2] >> 2 & 1;
        } else {
          mode |= c[2] >> 2 & 1;
        }
      }
      *output++ = p[mode << 7 | c[l]];
    }
  }
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
  for(auto& data : cram.palette) data = 0;

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

  static bool initialized = false;
  if(!initialized) {
    initialized = true;

    for(u32 a = 0; a < 32; a++) {
      for(u32 b = 0; b < 32; b++) {
        u32 ap = a & 1, ac = a >> 1;
        u32 bp = b & 1, bc = b >> 1;
        u32 bg = (ap && ac) || ac && !(bp && bc) ? 0 : bc ? 1 : 3;
        lookupBG[a << 5 | b] = bg;
      }
    }

    for(u32 a = 0; a < 32; a++) {
      for(u32 b = 0; b < 32; b++) {
        for(u32 s = 0; s < 32; s++) {
          u32 ap = a & 1, ac = a >> 1;
          u32 bp = b & 1, bc = b >> 1;
          u32 sp = s & 1, sc = s >> 1;
          u32 bg = (ap && ac) || ac && !(bp && bc) ? 0 : bc ? 1 : 3;
          u32 fg = (sp && sc) || sc && !(bp && bc) && !(ap && ac) ? 2 : bg;
          lookupFG[a << 10 | b << 5 | s] = fg;
        }
      }
    }
  }
}

}
