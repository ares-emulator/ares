#include <msx/msx.hpp>

namespace ares::MSX {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  if(Model::MSX()) {
    screen = node->append<Node::Video::Screen>("Screen", 284, 243);
    screen->colors(1 << 4, {&VDP::colorMSX, this});
    screen->setSize(284, 243);
    screen->setScale(1.0, 1.0);
    screen->setAspect(8.0, 7.0);
    TMS9918::vram.allocate(16_KiB, 0x00);
    TMS9918::load(screen);
  }

  if(Model::MSX2()) {
    screen = node->append<Node::Video::Screen>("Screen", 568, 486);
    screen->colors(1 << 9, {&VDP::colorMSX2, this});
    screen->setSize(568, 486);
    screen->setScale(0.5, 0.5);
    screen->setAspect(8.0, 7.0);
    V9938::vram.allocate(128_KiB, 0x00);
    V9938::xram.allocate(64_KiB, 0x00);
    V9938::pram.allocate(16);
    V9938::load(screen);
  }

  screen->refreshRateHint(system.colorburst(), 228, 262);
}

auto VDP::unload() -> void {
  if(Model::MSX())  TMS9918::unload();
  if(Model::MSX2()) V9938::unload();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto VDP::irq(bool line) -> void {
  cpu.setIRQ(line);
}

auto VDP::frame() -> void {
  int screenScale = Model::MSX2() ? 2 : 1;

  if(Model::MSX2()) {
    if(V9938::interlace() == 0) screen->setProgressive(1);
    if(V9938::interlace() == 1) screen->setInterlace(V9938::field());
  }

  if(screen->overscan()) {
    screen->setSize(284 * screenScale, 243 * screenScale);
    screen->setViewport(0, 0, 284 * screenScale, 243 * screenScale);
 } else {
    int x = 13;
    int y = 27;
    int width = 284 - 28;
    int height = 243 - 51;

    screen->setSize(width * screenScale, height * screenScale);
    screen->setViewport(x * screenScale, y * screenScale, width * screenScale, height * screenScale);
  }

  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power() -> void {
  screen->power();

  if(Model::MSX()) {
    TMS9918::power();
    Thread::create(system.colorburst() * 2, [&] { TMS9918::main(); });
  }

  if(Model::MSX2()) {
    V9938::power();
    Thread::create(system.colorburst() * 2, [&] { V9938::main(); });
  }
}

/* Z80 I/O ports 0x98 - 0x9b */

auto VDP::read(n2 port) -> n8 {
  if(Model::MSX())
  switch(port) {
  case 0: return TMS9918::data();
  case 1: return TMS9918::status();
  }

  if(Model::MSX2())
  switch(port) {
  case 0: return V9938::data();
  case 1: return V9938::status();
  }

  return 0xff;
}

auto VDP::write(n2 port, n8 data) -> void {
  if(Model::MSX())
  switch(port) {
  case 0: return TMS9918::data(data);
  case 1: return TMS9918::control(data);
  }

  if(Model::MSX2())
  switch(port) {
  case 0: return V9938::data(data);
  case 1: return V9938::control(data);
  case 2: return V9938::palette(data);
  case 3: return V9938::register(data);
  }
}

}
