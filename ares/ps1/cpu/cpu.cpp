#include <ps1/ps1.hpp>

namespace ares::PlayStation {

CPU cpu;
#include "core/core.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
}

auto CPU::unload() -> void {
  node.reset();
}

auto CPU::main() -> void {
}

auto CPU::step(uint clocks) -> void {
}

auto CPU::power(bool reset) -> void {
  Thread::reset();
  powerCore(reset);
}

}
