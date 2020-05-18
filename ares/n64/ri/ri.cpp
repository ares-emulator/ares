#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RI ri;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RI");
  debugger.load(node);
}

auto RI::unload() -> void {
  node = {};
  debugger = {};
}

auto RI::power() -> void {
}

}
