#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDP rdp;
#include "io-command.cpp"
#include "io-span.cpp"
#include "serialization.cpp"

auto RDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RDP");
}

auto RDP::unload() -> void {
  node = {};
}

auto RDP::main() -> void {
  step(93'750'000);
}

auto RDP::step(uint clocks) -> void {
  clock += clocks;
}

auto RDP::power() -> void {
  Thread::reset();
}

}
