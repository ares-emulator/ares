#include <ps1/ps1.hpp>

namespace ares::PlayStation {

GPU gpu;
#include "io.cpp"
#include "gp0.cpp"
#include "gp1.cpp"
#include "render.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("GPU");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1 << 16, [&](uint32 color) -> uint64 {
    u64 a = 65535;
    u64 r = image::normalize(color >>  0 & 31, 5, 16);
    u64 g = image::normalize(color >>  5 & 31, 5, 16);
    u64 b = image::normalize(color >> 10 & 31, 5, 16);
    return a << 48 | r << 32 | g << 16 | b << 0;
  });
//screen->setSize( 320, 240);
  screen->setSize( 640, 480);
//screen->setSize(1024, 512);

  vram.allocate(1_MiB);
}

auto GPU::unload() -> void {
  vram.reset();
  screen.reset();
  node.reset();
}

auto GPU::main() -> void {
  if(io.vcounter <= 240) {
    timer.hsync(1);
    timer.hsync(0);
  }

  if(io.vcounter == 240) {
    timer.vsync(1);
    interrupt.raise(Interrupt::Vblank);
  }

  if(++io.vcounter == 262) {
    timer.vsync(0);
    io.vcounter = 0;
    io.field = !io.field;
    interrupt.lower(Interrupt::Vblank);
    refreshed = true;
  }

  step(33'868'800 / 60 / 262);
}

auto GPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto GPU::refresh() -> void {
  u32 source = 0;
  u32 target = 0;
  for(uint y : range(512)) {
    for(uint x : range(1024)) {
      u32 data = vram.readHalf(source++ << 1);
      output[target++] = data & 0x7fff;
    }
  }
//screen->refresh((uint32*)output, 1024 * sizeof(uint32),  320, 240);
  screen->refresh((uint32*)output, 1024 * sizeof(uint32),  640, 480);
//screen->refresh((uint32*)output, 1024 * sizeof(uint32), 1024, 512);
}

auto GPU::power(bool reset) -> void {
  Thread::reset();
  refreshed = false;
  vram.fill();
  io = {};
}

}
