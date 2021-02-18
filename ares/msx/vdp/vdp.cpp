#include <msx/msx.hpp>

namespace ares::MSX {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  if(Model::MSX()) {
    screen_ = node->append<Node::Video::Screen>("Screen", 256, 192);
    screen_->colors(1 << 4, {&VDP::colorMSX, this});
    screen_->setSize(256, 192);
    screen_->setScale(1.0, 1.0);
    screen_->setAspect(1.0, 1.0);
    TMS9918::load(screen_);
  }

  if(Model::MSX2()) {
    screen_ = node->append<Node::Video::Screen>("Screen", 512, 424);
    screen_->colors(1 << 9, {&VDP::colorMSX2, this});
    screen_->setSize(512, 424);
    screen_->setScale(0.5, 0.5);
    screen_->setAspect(1.0, 1.0);
    V9938::load(screen_);
  }
}

auto VDP::unload() -> void {
  if(Model::MSX())  TMS9918::unload();
  if(Model::MSX2()) V9938::unload();
  screen_->quit();
  node->remove(screen_);
  screen_.reset();
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
  if(Model::MSX2()) {
    if(V9938::interlace() == 0) screen_->setProgressive(1);
    if(V9938::interlace() == 1) screen_->setInterlace(V9938::field());
  }
  screen_->setViewport(0, 0, screen_->width(), screen_->height());
  screen_->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power() -> void {
  if(Model::MSX()) {
    TMS9918::vram.allocate(16_KiB);
    TMS9918::power();
    Thread::create(system.colorburst() * 2, [&] { TMS9918::main(); });
  }

  if(Model::MSX2()) {
    V9938::videoRAM.allocate(128_KiB);
    V9938::expansionRAM.allocate(64_KiB);
    V9938::power();
    Thread::create(system.colorburst() * 2, [&] { V9938::main(); });
  }

  screen_->power();
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
