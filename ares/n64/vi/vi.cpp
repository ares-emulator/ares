#include <n64/n64.hpp>

namespace ares::Nintendo64 {

VI vi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("VI");

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
  screen->setSize(640, 478);

  debugger.load(node);
}

auto VI::unload() -> void {
  node = {};
  screen = {};
  debugger = {};
}

auto VI::main() -> void {
  if(io.vcounter == io.coincidence >> 1) {
    mi.raise(MI::IRQ::VI);
  }

  if(++io.vcounter == 262) {
    io.vcounter = 0;
    refreshed = true;
  }

  step(93'750'000 / 60 / 262);
}

auto VI::step(uint clocks) -> void {
  clock += clocks;
}

auto VI::refresh() -> void {
  uint pitch  = vi.io.width;
  uint width  = vi.io.width;  //vi.io.xscale <= 0x300 ? 320 : 640;
  uint height = vi.io.yscale <= 0x400 ? 239 : 478;

  if(vi.io.colorDepth < 2) {
    memory::fill<u32>(output, width * height);
  }

  if(vi.io.colorDepth == 2) {
    //15bpp
    for(uint y : range(height)) {
      u32 address = vi.io.dramAddress + y * pitch * 2;
      auto line = &output[y * width];
      for(uint x : range(min(width, pitch))) {
        u16 data = bus.readHalf(address + x * 2);
        *line++ = 1 << 24 | data >> 1;
      }
    }
  }

  if(vi.io.colorDepth == 3) {
    //24bpp
    for(uint y : range(height)) {
      u32 address = vi.io.dramAddress + y * pitch * 4;
      auto line = &output[y * width];
      for(uint x : range(min(width, pitch))) {
        u32 data = bus.readWord(address + x * 4);
        *line++ = data >> 8;
      }
    }
  }

  screen->refresh((uint32*)output, width * sizeof(uint32), width, height);
}

auto VI::power() -> void {
  Thread::reset();
  refreshed = false;
  io = {};
}

}
