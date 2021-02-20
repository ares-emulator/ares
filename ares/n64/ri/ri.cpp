#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RI ri;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RI");
  debugger.load(node);
}

auto RI::unload() -> void {
  debugger = {};
  node.reset();
}

auto RI::power(bool reset) -> void {
  io = {};
}

}
