#include <ng/ng.hpp>

namespace ares::NeoGeo {

#include "slot.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

Card::Card(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Memory Card");
  ram.allocate(2_KiB);
  debugger.load(node);
}

Card::~Card() {
  debugger.unload(node);
  ram.reset();
  node.reset();
}

auto Card::power(bool reset) -> void {
}

}
