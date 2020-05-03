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
  scheduler.exit(Event::Frame);
  step(1);
}

auto RDP::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto RDP::refresh() -> void {
  if(vi.io.colorDepth < 2) {
    memory::fill<u32>(output, 320 * 240);
  }

  if(vi.io.colorDepth == 2) {
    //15bpp
    for(uint y : range(240)) {
      u32 address = vi.io.dramAddress + y * 320 * 2;
      for(uint x : range(320)) {
        u16 data = bus.readHalf(address + x * 2);
        output[y * 320 + x] = 1 << 24 | data >> 1;
      }
    }
  }

  if(vi.io.colorDepth == 3) {
    //24bpp
    for(uint y : range(240)) {
      u32 address = vi.io.dramAddress + y * 320 * 4;
      for(uint x : range(320)) {
        u32 data = bus.readWord(address + x * 4);
        output[y * 320 + x] = data >> 8;
      }
    }
  }

  screen->refresh(output, 320 * sizeof(uint32), 320, 240);
}

auto RDP::power() -> void {
  Thread::create(60, {&RDP::main, this});
}

}
