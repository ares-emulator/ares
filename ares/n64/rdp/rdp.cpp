#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDP rdp;
#include "render.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RDP");
  debugger.load(node);
}

auto RDP::unload() -> void {
  node = {};
  debugger = {};
}

auto RDP::main() -> void {
  step(93'750'000);
}

auto RDP::step(uint clocks) -> void {
  clock += clocks;
}

auto RDP::power(bool reset) -> void {
  Thread::reset();
  command.ready = 1;
}

}
