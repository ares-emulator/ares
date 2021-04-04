#include <md/md.hpp>

namespace ares::MegaDrive {

Bus bus;
#include "serialization.cpp"

auto Bus::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Bus");
}

auto Bus::unload() -> void {
  node.reset();
}

auto Bus::power(bool reset) -> void {
  state = {};
}

}
