#include <n64/n64.hpp>

namespace ares::Nintendo64 {

CPU cpu;
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
}

auto CPU::unload() -> void {
  node = {};
}

auto CPU::main() -> void {
  step(20);
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power() -> void {
  VR4300::power();
  Thread::create(93'750'000, {&CPU::main, this});
}

}
