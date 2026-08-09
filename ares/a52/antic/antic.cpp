#include <a52/a52.hpp>

namespace ares::Atari5200 {

ANTIC antic;

auto ANTIC::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("ANTIC");
}

auto ANTIC::unload() -> void {
  Thread::destroy();
  node = {};
}

auto ANTIC::main() -> void {
  Thread::step(Timing::ColorClocksPerScanline);
  Thread::synchronize(cpu);
  if(++scanline == Timing::ScanlinesPerFrame) {
    scanline = 0;
    scheduler.exit(Event::Frame);
  }
}

auto ANTIC::power() -> void {
  Thread::create(system.frequency(), std::bind_front(&ANTIC::main, this));
  scanline = 0;
}

auto ANTIC::read(n8 address) -> n8 {
  return peek(address);
}

auto ANTIC::peek(n8 address) const -> n8 {
  return 0xff;
}

auto ANTIC::write(n8 address, n8 data) -> void {
}

}
