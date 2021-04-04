#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.cpp"
#else
#include <md/md.hpp>

namespace ares::MegaDrive {

VDP vdp;
#include "main.cpp"
#include "fifo.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "render.cpp"
#include "background.cpp"
#include "sprite.cpp"
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

  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger.unload();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency() / 2.0, {&VDP::main, this});
  screen->power();

  if(!reset) {
    for(auto& data : vram.memory ) data = 0;
    for(auto& data : vsram.memory) data = 0;
    for(auto& data : cram.memory ) data = 0;
  }

  fifo.slots.flush();
  vram.mode = 0;
  io = {};
  latch = {};
  state = {};

  planeA.power();
  window.power();
  planeB.power();
  sprite.power();
  dma.power();
}

}
#endif
