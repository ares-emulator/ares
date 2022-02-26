#include <md/md.hpp>

namespace ares::MegaDrive {

VDP vdp;
#include "psg.cpp"
#include "main.cpp"
#include "irq.cpp"
#include "render.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "background.cpp"
#include "object.cpp"
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

  psg.load(node);
  debugger.load(node);
}

auto VDP::unload() -> void {
  debugger = {};
  psg.unload();
  overscan.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::frame() -> void {
  if(latch.interlace == 0) screen->setProgressive(1);
  if(latch.interlace == 1) screen->setInterlace(field());
  screen->setViewport(0, 0, screen->width(), screen->height());
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power(bool reset) -> void {
  Thread::create(system.frequency() / 2.0, {&VDP::main, this});
  screen->power();

  for(auto& data : vram.pixels) data = 0;
  for(auto& data : vram.memory) data = 0;
  vram.size = 32768;
  vram.mode = 0;

  for(auto& data : vsram.memory) data = 0;

  for(auto& data : cram.memory) data = 0;

  psg.power(reset);
  irq.power(reset);
  dma.power();

  planeA.io = {};
  window.io = {};
  planeB.io = {};

  sprite.io = {};
  for(auto& object : sprite.oam) object = {};
  for(auto& object : sprite.objects) object = {};

  command = {};
  io = {};
  latch = {};
  state = {};
}

}
