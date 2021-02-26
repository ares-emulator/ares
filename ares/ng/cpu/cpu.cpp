#include <ng/ng.hpp>

namespace ares::NeoGeo {

CPU cpu;
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
}

auto CPU::unload() -> void {
  node.reset();
}

auto CPU::main() -> void {
  step(frequency());
}

auto CPU::step(u32 clocks) -> void {
  Thread::step(clocks);
}

auto CPU::power(bool reset) -> void {
  M68K::power();
  Thread::create(12'000'000, {&CPU::main, this});
}

}
