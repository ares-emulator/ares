#include <ng/ng.hpp>

namespace ares::NeoGeo {

APU apu;
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");
}

auto APU::unload() -> void {
  node.reset();
}

auto APU::main() -> void {
  step(frequency());
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
}

auto APU::power(bool reset) -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(4'000'000, {&APU::main, this});
}

}
