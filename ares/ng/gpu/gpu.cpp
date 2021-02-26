#include <ng/ng.hpp>

namespace ares::NeoGeo {

GPU gpu;
#include "color.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GPU");

  screen = node->append<Node::Video::Screen>("Screen", 640, 480);
  screen->colors(1, {&GPU::color, this});
  screen->setSize(640, 480);
  screen->setSize(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
}

auto GPU::unload() -> void {
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto GPU::main() -> void {
  step(1);
  screen->setViewport(0, 0, 640, 480);
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto GPU::step(u32 clocks) -> void {
  Thread::step(clocks);
}

auto GPU::power(bool reset) -> void {
  Thread::create(60, {&GPU::main, this});
  screen->power();
}

}
