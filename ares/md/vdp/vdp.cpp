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

  screen = node->append<Node::Video::Screen>("Screen", 1415, visibleHeight() * 2);
  screen->colors(1 << 16, {&VDP::color, this});
  screen->setSize(1415, visibleHeight() * 2);
  screen->setScale(0.25, 0.5);
  Region::PAL() ? screen->setAspect(41.0, 37.0) : screen->setAspect(32.0, 35.0);
  screen->refreshRateHint(system.frequency(), 3420, frameHeight());

  psg.load(node);
  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger.unload();
  psg.unload();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::updateScreenParams() -> void {
  if(Region::NTSC() && v28()) { state.topline = 0x1e5; state.bottomline = 0x0ea; }
  if(Region::NTSC() && v30()) { state.topline = 0x000; state.bottomline = 0x1ff; }
  if(Region::PAL()  && v28()) { state.topline = 0x1ca; state.bottomline = 0x102; }
  if(Region::PAL()  && v30()) { state.topline = 0x1d2; state.bottomline = 0x10a; }
}

auto VDP::pixels() -> u32* {
  u32* output = nullptr;
  n9 y = vcounter();

  // disregard blanked lines
  if(Region::NTSC() &&                    y >= 0x0e8 && y < 0x1f5) return nullptr;
  if(Region::PAL()  &&  latch.overscan && y >= 0x108 && y < 0x1e2) return nullptr;
  if(Region::PAL()  && !latch.overscan && y >= 0x100 && y < 0x1da) return nullptr;

  // adjust vcounter to account for vsync period & vcounter jump
  if(Region::NTSC() && y >= 0x0e8) y -= (0x1f5 - 0x0e8);
  if(Region::PAL()  && y >= 0x108) y -= (0x1e2 - 0x108);

  // adjust for top border
  if(Region::NTSC()) y += 11;
  if(Region::PAL() ) y += 38 - 8 * latch.overscan;
  y = y % visibleHeight();

  auto yScale = latch.interlace ? 2 : 1;
  output = screen->pixels().data() + y * yScale * 1415;
  if(latch.interlace) output += field() * 1415;

  if(h40()) {
    // H40 mode has slightly shorter lines, so sides are blanked.
    // Left side would be 13 wide, but we'll realign to whole pixel (3*4) for sanity.
    for(auto n: range(12)) output[        n] = 0b101 << 9;
    for(auto n: range(15)) output[1415-15+n] = 0b101 << 9;

    return output+12;
  }

  return output;
}

auto VDP::frame() -> void {
  if(latch.interlace == 0) screen->setProgressive(0);
  if(latch.interlace == 1) screen->setInterlace(field());
  auto yScale = latch.interlace ? 2 : 1;
  screen->setScale(0.25, 1.0 / yScale);

  if(screen->overscan()) {
    screen->setSize(1415, visibleHeight() * yScale);
    screen->setViewport(0, 0, screen->width(), screen->height());
  } else {
    int x = 13 * 5;
    int y = Region::PAL() ? 30 + 8 * v28() : 11;
    int width = 1280;
    int height = screenHeight();

    screen->setSize(width, height * yScale);
    screen->setViewport(x, y * yScale, width, height * yScale);
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
  cram.bus.active = 0;
  cram.bus.data = 0;
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
