#if 0 //defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.cpp"
#else
#include <md/md.hpp>

namespace ares::MegaDrive {

//flips 4-bit nibble ordering in 8-pixel row
inline auto hflip(u32 data) -> u32 {
  data = data >> 16 & 0x0000ffff | data << 16 & 0xffff0000;
  data = data >>  8 & 0x00ff00ff | data <<  8 & 0xff00ff00;
  data = data >>  4 & 0x0f0f0f0f | data <<  4 & 0xf0f0f0f0;
  return data;
}

VDP vdp;
#include "psg.cpp"
#include "main.cpp"
#include "irq.cpp"
#include "prefetch.cpp"
#include "fifo.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "layers.cpp"
#include "window.cpp"
#include "layer.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 1388, visibleHeight() * 2);
  screen->colors(1 << 16, {&VDP::color, this});
  screen->setSize(1388, visibleHeight() * 2);
  screen->setScale(0.25, 0.5);
  Region::PAL() ? screen->setAspect(111.0, 100.0) : screen->setAspect(32.0, 35.0);
  screen->refreshRateHint(system.frequency(), 3420, frameHeight());

  psg.load(node);
  debugger.load(node);

  generateCycleTimings();
}

auto VDP::unload() -> void {
  debugger.unload();
  psg.unload();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::pixels() -> u32* {
  //TODO: vcounter values of top border may not be correct here

  u32* output = nullptr;
  if(Region::NTSC() && vcounter() >= 0x1ed) return nullptr;
  if(Region::PAL()  && vcounter() >= 0x1f0) return nullptr;

  //account for vcounter jumps during blanking periods
  n9 y = vcounter();
  if(Region::NTSC() && v28() && vcounter() >= 0x1e5) y -= 250;
  if(Region::PAL()  && v28() && vcounter() >= 0x1ca) y -= 201;
  if(v30() && Region::PAL()  && vcounter() >= 0x1d2) y -= 201;

  auto offset = Region::PAL() ? 38 : 11;
  if(latch.overscan) offset -= 8;

  y = (y + offset) % visibleHeight();

  output = screen->pixels().data() + y * 2 * 1388;
  if(latch.interlace) output += field() * 1388;

  //TODO: this should probably be handled in DAC
  n32 bg = 1 << 11 | 1 << 9 | cram.color(io.backgroundColor);
  for(auto n: range(1388)) output[n] = bg;

  return output + 52;
}

auto VDP::frame() -> void {
  if(latch.interlace == 0) screen->setProgressive(1);
  if(latch.interlace == 1) screen->setInterlace(field());

  if(screen->overscan()) {
    screen->setSize(1388, visibleHeight() * 2);
    screen->setViewport(0, 0, screen->width(), screen->height());
  } else {
    int x = 14 * 4;
    int y = 12 * 2;
    int width = 1388 - (28 * 4);
    int height = (visibleHeight() * 2) - (24 * 2);

    if(Region::PAL()) {
      y += 28 * 2;
      height -= 48 * 2;

      if(v30()) {
        y -= 8 * 2;
        height += 16 * 2;
      }
    }

    screen->setSize(width, height);
    screen->setViewport(x, y, width, height);
  }

  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency(), {&VDP::main, this});
  screen->power();

  if(!reset) {
    for(auto& data : vram.memory ) data = 0;
    for(auto& data : vsram.memory) data = 0;
    for(auto& data : cram.memory ) data = 0;
  }

  vram.mode = 0;
  vram.refreshing = 0;
  command = {};
  io = {};
  test = {};
  latch = {};
  state = {};

  psg.power(reset);
  irq.power(reset);
  prefetch.power(reset);
  fifo.power(reset);
  dma.power(reset);
  layers.power(reset);
  window.power(reset);
  layerA.power(reset);
  layerB.power(reset);
  sprite.power(reset);
  dac.power(reset);
}

}
#endif
