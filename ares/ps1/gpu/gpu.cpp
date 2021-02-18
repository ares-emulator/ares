#include <ps1/ps1.hpp>

namespace ares::PlayStation {

//cpu.clock = 44,100 * 768 = 33,868,000hz
//gpu.clock = cpu.clock * 11 / 7 = 53,222,400hz
//ntsc.clocks_per_scanline = ~3,413
// pal.clocks_per_scanline = ~3,406
//ntsc.scanlines_per_frame = 263
// pal.scanlines_per_frame = 314

GPU gpu;
GPU::Color GPU::Color::table[65536];
#include "io.cpp"
#include "gp0.cpp"
#include "gp1.cpp"
#include "renderer.cpp"
#include "blitter.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GPU");

  screen = node->append<Node::Video::Screen>("Screen", 640, 512);
  screen->setRefresh({&GPU::Blitter::refresh, &blitter});
  screen->colors((1 << 24) + (1 << 15), [&](n32 color) -> n64 {
    if(color < (1 << 24)) {
      u64 a = 65535;
      u64 r = image::normalize(color >>  0 & 255, 8, 16);
      u64 g = image::normalize(color >>  8 & 255, 8, 16);
      u64 b = image::normalize(color >> 16 & 255, 8, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    } else {
      u64 a = 65535;
      u64 r = image::normalize(color >>  0 & 31, 5, 16);
      u64 g = image::normalize(color >>  5 & 31, 5, 16);
      u64 b = image::normalize(color >> 10 & 31, 5, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    }
  });
  screen->setSize(640, 512);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(640, 480);
    if(value == 1) screen->setSize(640, 512);
  });
  overscan->setDynamic(true);

  vram.allocate(1_MiB);
  for(u32 y : range(512)) {
    vram2D[y] = (u16*)&vram.data[y * 2048];
  }

  debugger.load(node);

  generateTables();
}

auto GPU::unload() -> void {
  renderer.kill();
  debugger = {};
  vram.reset();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto GPU::main() -> void {
  step(12);

  if(io.hcounter == 1800) {
    //hsync signal is sent even during vertical blanking period
    timer.hsync(1);
  }

  if(io.hcounter == 2172) {
    io.hcounter = 0;
    timer.hsync(0);

    if(++io.vcounter == vtotal()) {
      io.vcounter = 0;
      io.field = !io.field;
      frame();
    }

    if(io.vcounter == vstart()) {
      timer.vsync(0);
      interrupt.lower(Interrupt::Vblank);
    }

    if(io.vcounter == vend()) {
      timer.vsync(1);
      io.interrupt = 1;
      interrupt.raise(Interrupt::Vblank);
      blitter.queue();
    }
  }
}

auto GPU::frame() -> void {
  switch(io.horizontalResolution) {
  case 0:
    display.dotclock = 10;
    display.width = 256;
    break;
  case 1:
    display.dotclock = 8;
    display.width = 320;
    break;
  case 2:
    display.dotclock = 5;
    display.width = 512;
    break;
  case 3:
    display.dotclock = 4;
    display.width = 640;
    break;
  case 4: case 5: case 6: case 7:
    display.dotclock = 7;
    display.width = 368;
    break;
  }

  if(io.verticalResolution && io.interlace) {
    display.height = io.videoMode ? 512 : 480;
    display.interlace = 1;
  } else {
    display.height = io.videoMode ? 256 : 240;
    display.interlace = 0;
  }
}

auto GPU::step(u32 clocks) -> void {
  Thread::clock += clocks;
  io.hcounter += clocks;
  io.pcounter -= clocks;
  if(io.pcounter < 0) io.pcounter = 0;
}

auto GPU::power(bool reset) -> void {
  Thread::reset();
  Memory::Interface::setWaitStates(2, 2, 3);
  screen->power();
  refreshed = false;

  vram.fill();
  display.dotclock = 0;
  display.width = 0;
  display.height = 0;
  display.interlace = 0;
  display.previous.x = 0;
  display.previous.y = 0;
  display.previous.width = 0;
  display.previous.height = 0;
  io = {};
  queue.gp0 = {};
  queue.gp1 = {};

  frame();
  renderer.power();
  blitter.power();
}

}
