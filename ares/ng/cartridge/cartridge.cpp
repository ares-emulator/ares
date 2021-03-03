#include <ng/ng.hpp>

namespace ares::NeoGeo {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Neo Geo Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
}

auto Cartridge::power() -> void {
}

}
