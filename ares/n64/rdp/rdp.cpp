#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDP rdp;
#include "serialization.cpp"

auto RDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RDP");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1, [&](uint32 color) -> uint64 { return ~0; });
  screen->setSize(640, 480);
}

auto RDP::unload() -> void {
  node = {};
  screen = {};
}

auto RDP::main() -> void {
  scheduler.exit(Event::Frame);
  step(1);
}

auto RDP::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto RDP::refresh() -> void {
  screen->refresh(output, 640 * sizeof(uint32), 640, 480);
}

auto RDP::power() -> void {
  Thread::create(60, {&RDP::main, this});
}

}
