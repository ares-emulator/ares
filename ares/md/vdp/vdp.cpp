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

  screen = node->append<Node::Video::Screen>("Screen", 1280, 480);
  screen->colors(1 << 16, {&VDP::color, this});
  screen->setSize(1280, 480);
  screen->setScale(0.25, 0.5);
  screen->setAspect(1.0, 1.0);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(1280, 448);
    if(value == 1) screen->setSize(1280, 480);
  });
  overscan->setDynamic(true);

  psg.load(node);
  debugger.load(node);

  generateCycleTimings();
}

auto VDP::unload() -> void {
  debugger.unload();
  psg.unload();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::pixels() -> u32* {
  u32* output = nullptr;
  if(overscan->value() == 0 && latch.overscan == 0) {
    if(vcounter() >= 224) return nullptr;
    output = screen->pixels().data() + (vcounter() - 0) * 2 * 1280;
  }
  if(overscan->value() == 0 && latch.overscan == 1) {
    if(vcounter() <=   7) return nullptr;
    if(vcounter() >= 232) return nullptr;
    output = screen->pixels().data() + (vcounter() - 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 0) {
    if(vcounter() >= 232) return nullptr;
    output = screen->pixels().data() + (vcounter() + 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 1) {
    output = screen->pixels().data() + (vcounter() + 0) * 2 * 1280;
  }
  if(latch.interlace) output += field() * 1280;
  return output;
}

auto VDP::frame() -> void {
  if(latch.interlace == 0) screen->setProgressive(1);
  if(latch.interlace == 1) screen->setInterlace(field());
  screen->setViewport(0, 0, screen->width(), screen->height());
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
