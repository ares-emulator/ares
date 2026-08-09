#include <a52/a52.hpp>

namespace ares::Atari5200 {

GTIA gtia;

auto GTIA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GTIA");
}

auto GTIA::unload() -> void {
  node = {};
}

auto GTIA::power() -> void {
}

auto GTIA::read(n8 address) -> n8 {
  return peek(address);
}

auto GTIA::peek(n8 address) const -> n8 {
  address &= 0x1f;
  if(address <= 0x13) return 0x00;
  return 0x0f;
}

auto GTIA::write(n8 address, n8 data) -> void {
}

}
