#include <sg/sg.hpp>

namespace ares::SG1000 {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(16_KiB, 0x00);

  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 256, 192);
  screen->colors(1 << 4, {&VDP::color, this});
  screen->setSize(256, 192);
  screen->setViewport(0, 0, 256, 192);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);

  TMS9918::load(screen);
}

auto VDP::unload() -> void {
  TMS9918::unload();
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
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power() -> void {
  TMS9918::power();
  Thread::create(system.colorburst() * 2, [&] { main(); });
  screen->power();
}

}
