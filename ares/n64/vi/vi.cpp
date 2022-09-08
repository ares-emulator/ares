#include <n64/n64.hpp>

namespace ares::Nintendo64 {

VI vi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VI");

  u32 width = 640;
  u32 height = 576;

  #if defined(VULKAN)
  if (vulkan.enable) {
    width *= vulkan.outputUpscale;
    height *= vulkan.outputUpscale;
  }
  #endif
  screen = node->append<Node::Video::Screen>("Screen", width, height);
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
  
  #if defined(VULKAN)
  if(vulkan.enable) {
    screen->setSize(vulkan.outputUpscale * 640, vulkan.outputUpscale * 480);
    if(!vulkan.supersampleScanout) {
      screen->setScale(1.0 / vulkan.outputUpscale, 1.0 / vulkan.outputUpscale);
    }
  } else {
    screen->setSize(640, 480);
  }
  #else
  screen->setSize(640, 480);
  #endif

  debugger.load(node);
}

auto VI::unload() -> void {
  debugger = {};
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VI::main() -> void {
  //field is not compared
  if(io.vcounter << 1 == io.coincidence) {
    mi.raise(MI::IRQ::VI);
  }

  if(++io.vcounter >= (Region::NTSC() ? 262 : 312) + io.field) {
    io.vcounter = 0;
    io.field = io.field + 1 & io.serrate;
    if(!io.field) {
      #if defined(VULKAN)
      if (vulkan.enable) {
        gpuOutputValid = vulkan.scanoutAsync(io.field);
        vulkan.frame();
      }
      #endif

      refreshed = true;
      screen->frame();
    }
  }

  if(Region::NTSC()) step(system.frequency() / 60 / 262);
  if(Region::PAL ()) step(system.frequency() / 50 / 312);
}

auto VI::step(u32 clocks) -> void {
  Thread::clock += clocks;
}

auto VI::refresh() -> void {
  #if defined(VULKAN)
  if(vulkan.enable && gpuOutputValid) {
    const u8* rgba = nullptr;
    u32 width = 0, height = 0;
    vulkan.mapScanoutRead(rgba, width, height);
    if(rgba) {
      screen->setViewport(0, 0, width, height);
      for(u32 y : range(height)) {
        auto source = rgba + width * y * sizeof(u32);
        auto target = screen->pixels(1).data() + y * vulkan.outputUpscale * 640;
        for(u32 x : range(width)) {
          target[x] = source[x * 4 + 0] << 16 | source[x * 4 + 1] << 8 | source[x * 4 + 2] << 0;
        }
      }
    } else {
      screen->setViewport(0, 0, 1, 1);
      screen->pixels(1).data()[0] = 0;
    }
    vulkan.unmapScanoutRead();
    vulkan.endScanout();
    return;
  }
  #endif

  if(io.serrate == 0) screen->setProgressive(0);
  if(io.serrate == 1) screen->setInterlace(io.field);

  u32 hscan_start = Region::NTSC() ? 108 : 128;
  u32 vscan_start = Region::NTSC() ?  34 :  44;
  u32 hscan_len   = Region::NTSC() ? 640 : 640;
  u32 vscan_len   = Region::NTSC() ? 480 : 576;
  u32 hscan_stop  = hscan_start + hscan_len;
  u32 vscan_stop  = vscan_start + vscan_len;
  screen->setViewport(0, 0, hscan_len, vscan_len);

  u32 dy0 = max(vscan_start, vi.io.vstart + io.field*2);
  u32 dy1 = min(vscan_stop,  vi.io.vend   + io.field*2);
  u32 dx0 = max(hscan_start, vi.io.hstart);
  u32 dx1 = min(hscan_stop,  vi.io.hend);

  u32 pitch = vi.io.width;
  if(vi.io.colorDepth == 2) {
    //15bpp
    u32 y0 = vi.io.ysubpixel + vi.io.yscale * (dy0 - (vi.io.vstart + io.field*2));
    for(u32 dy = dy0; dy < dy1; dy++) {
      u32 address = vi.io.dramAddress + (y0 >> 11) * pitch * 2;
      auto line = screen->pixels(1).data() + (dy - vscan_start) * hscan_len;
      u32 x0 = vi.io.xsubpixel + vi.io.xscale * (dx0 - vi.io.hstart);
      for(u32 dx = dx0; dx < dx1; dx++) {
        u16 data = bus.read<Half>(address + (x0 >> 10) * 2);
        line[dx - hscan_start] = 1 << 24 | data >> 1;
        x0 += vi.io.xscale;
      }
      y0 += vi.io.yscale;
    }
  }

  if(vi.io.colorDepth == 3) {
    //24bpp
    u32 y0 = vi.io.ysubpixel + vi.io.yscale * (dy0 - (vi.io.vstart + io.field*2));
    for(u32 dy = dy0; dy < dy1; dy++) {
      u32 address = vi.io.dramAddress + (y0 >> 11) * pitch * 4;
      auto line = screen->pixels(1).data() + (dy - vscan_start) * hscan_len;
      u32 x0 = vi.io.xsubpixel + vi.io.xscale * (dx0 - vi.io.hstart);
      for(u32 dx = dx0; dx < dx1; dx++) {
        u32 data = bus.read<Word>(address + (x0 >> 10) * 4);
        line[dx - hscan_start] = data >> 8;
        x0 += vi.io.xscale;
      }
      y0 += vi.io.yscale;
    }
  }
}

auto VI::power(bool reset) -> void {
  Thread::reset();
  screen->power();
  io = {};
  refreshed = false;

  #if defined(VULKAN)
  gpuOutputValid = false;
  #endif
}

}
