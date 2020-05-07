#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDP rdp;
#include "io-command.cpp"
#include "io-span.cpp"
#include "serialization.cpp"

auto RDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RDP");

  screen = node->append<Node::Screen>("Screen");
  screen->colors((1 << 24) + (1 << 15), [&](uint32 color) -> uint64 {
    if(color < (1 << 24)) {
      u64 a = 65535;
      u64 r = image::normalize(color >> 16 & 255, 8, 16);
      u64 g = image::normalize(color >>  8 & 255, 8, 16);
      u64 b = image::normalize(color >>  0 & 255, 8, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    } else {
      u64 a = 65535;
      u64 r = image::normalize(color >> 10 & 31, 5, 16);
      u64 g = image::normalize(color >>  5 & 31, 5, 16);
      u64 b = image::normalize(color >>  0 & 31, 5, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    }
  });
  screen->setSize(640, 480);
}

auto RDP::unload() -> void {
  node = {};
  screen = {};
}

auto RDP::main() -> void {
  io.vcounter++;

  if(io.vcounter == 240) {
    mi.irq.vi.line = 1;
    mi.pollInterrupts();
  }

  if(io.vcounter == 262) {
    io.vcounter = 0;
    refreshed = true;
  }

  step(93'750'000 / 60 / 262);
}

auto RDP::step(uint clocks) -> void {
  clock += clocks;
}

auto RDP::refresh() -> void {
  uint width  = vi.io.xscale == 0x200 ? 320 : 640;
  uint height = vi.io.yscale == 0x400 ? 240 : 480;

  if(vi.io.colorDepth < 2) {
    memory::fill<u32>(output, width * height);
  }

  if(vi.io.colorDepth == 2) {
    //15bpp
    for(uint y : range(height)) {
      u32 address = vi.io.dramAddress + y * width * 2;
      auto line = &output[y * width];
      for(uint x : range(width)) {
        u16 data = bus.readHalf(address + x * 2);
        *line++ = 1 << 24 | data >> 1;
      }
    }
  }

  if(vi.io.colorDepth == 3) {
    //24bpp
    for(uint y : range(height)) {
      u32 address = vi.io.dramAddress + y * width * 4;
      auto line = &output[y * width];
      for(uint x : range(width)) {
        u32 data = bus.readWord(address + x * 4);
        *line++ = data >> 8;
      }
    }
  }

  screen->refresh((uint32*)output, width * sizeof(uint32), width, height);
}

auto RDP::power() -> void {
  Thread::reset();
  refreshed = false;
  io.vcounter = 0;
  io.field = 0;
}

}
