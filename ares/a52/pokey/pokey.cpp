#include <a52/a52.hpp>

namespace ares::Atari5200 {

POKEY pokey;

auto POKEY::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("POKEY");
}

auto POKEY::unload() -> void {
  node = {};
}

auto POKEY::power() -> void {
}

auto POKEY::read(n8 address) -> n8 {
  return peek(address);
}

auto POKEY::peek(n8 address) const -> n8 {
  return 0xff;
}

auto POKEY::write(n8 address, n8 data) -> void {
}

}
