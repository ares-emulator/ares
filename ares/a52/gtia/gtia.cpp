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
  // Register side effects arrive with the GTIA implementation.
}

auto GTIA::clock(n3 an) -> void {
}

auto GTIA::loadPlayerDMA(u8 player, n8 data, u32 scanline) -> void {
}

auto GTIA::loadMissileDMA(n8 data, u32 scanline) -> void {
}

auto GTIA::frame() -> void {
  scheduler.exit(Event::Frame);
}

}
