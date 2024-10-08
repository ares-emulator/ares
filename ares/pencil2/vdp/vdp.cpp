#include <pencil2/pencil2.hpp>

namespace ares::Pencil2 {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(16_KiB, 0x00);

  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", 284, 243);
  screen->colors(1 << 4, {&VDP::color, this});
  screen->setSize(284, 243);
  screen->setScale(1.0, 1.0);
  screen->setAspect(8.0, 7.0);
  screen->setViewport(0, 0, 284, 243);
  screen->refreshRateHint(system.crystalFrequency() / 2.0, 228, 262);

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
  cpu.setNMI(line);
}

auto VDP::frame() -> void {
  if(screen->overscan()) {
    screen->setSize(284, 243);
    screen->setViewport(0, 0, 284, 243);
  } else {
    int x = 16;
    int y = 16;
    int width = 284 - 32;
    int height = 243 - 32;

    screen->setSize(width, height);
    screen->setViewport(x, y, width, height);
  }

  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power() -> void {
  TMS9918::power();
  Thread::create(system.crystalFrequency() / 2.0, [&] { main(); });
  screen->power();
}

}
