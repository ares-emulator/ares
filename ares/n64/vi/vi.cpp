#include <n64/n64.hpp>

namespace ares::Nintendo64 {

VI vi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VI");

  screen = node->append<Node::Video::Screen>("Screen", 640, 478);
  screen->setRefresh({&VI::refresh, this});
  screen->colors((1 << 24) + (1 << 15), [&](n32 color) -> n64 {
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
  screen->quit();
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

    #if defined(VULKAN)
    gpuOutputValid = vulkan.scanout(gpuColorBuffer, gpuOutputWidth, gpuOutputHeight);
    vulkan.frame();
    #endif

    screen->frame();
  }

  step(93'750'000 / 60 / 262);
}

auto VI::step(u32 clocks) -> void {
  clock += clocks;
}

auto VI::refresh() -> void {
  #if defined(VULKAN)
  if(gpuOutputValid) {
    if(!gpuColorBuffer.empty()) {
      screen->setViewport(0, 0, gpuOutputWidth, gpuOutputHeight);
      for(u32 y : range(gpuOutputHeight)) {
        auto target = screen->pixels(1).data() + y * 640;
        auto source = gpuColorBuffer.data() + gpuOutputWidth * y;
        for(u32 x : range(gpuOutputWidth)) {
          target[x] = source[x].r << 16 | source[x].g << 8 | source[x].b << 0;
        }
      }
    } else {
      screen->setViewport(0, 0, 1, 1);
      screen->pixels(1).data()[0] = 0;
    }
    return;
  }
  #endif

  u32 pitch  = vi.io.width;
  u32 width  = vi.io.width;  //vi.io.xscale <= 0x300 ? 320 : 640;
  u32 height = vi.io.yscale <= 0x400 ? 239 : 478;
  screen->setViewport(0, 0, width, height);

  if(vi.io.colorDepth == 2) {
    //15bpp
    for(u32 y : range(height)) {
      u32 address = vi.io.dramAddress + y * pitch * 2;
      auto line = screen->pixels(1).data() + y * 640;
      for(u32 x : range(min(width, pitch))) {
        u16 data = bus.readHalf(address + x * 2);
        *line++ = 1 << 24 | data >> 1;
      }
    }
  }

  if(vi.io.colorDepth == 3) {
    //24bpp
    for(u32 y : range(height)) {
      u32 address = vi.io.dramAddress + y * pitch * 4;
      auto line = screen->pixels(1).data() + y * 640;
      for(u32 x : range(min(width, pitch))) {
        u32 data = bus.readWord(address + x * 4);
        *line++ = data >> 8;
      }
    }
  }
}

auto VI::power(bool reset) -> void {
  Thread::reset();
  screen->power();
  refreshed = false;
}

}
