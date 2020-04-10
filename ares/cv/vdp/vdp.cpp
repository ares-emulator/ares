#include <cv/cv.hpp>

namespace ares::ColecoVision {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("VDP");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1 << 4, {&VDP::color, this});
  screen->setSize(256, 192);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
}

auto VDP::unload() -> void {
  node = {};
  screen = {};
}

auto VDP::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto VDP::irq(bool line) -> void {
  cpu.setNMI(line);
}

auto VDP::frame() -> void {
  scheduler.exit(Event::Frame);
}

auto VDP::refresh() -> void {
  screen->refresh(buffer, 256 * sizeof(uint32), 256, 192);
}

auto VDP::power() -> void {
  TMS9918::power();
  Thread::create(system.colorburst() * 2, [&] { main(); });
  vram.allocate(0x4000);
}

}
