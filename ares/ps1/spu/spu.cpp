#include <ps1/ps1.hpp>

namespace ares::PlayStation {

SPU spu;
#include "serialization.cpp"

auto SPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("SPU");
}

auto SPU::unload() -> void {
  node.reset();
}

auto SPU::main() -> void {
}

auto SPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto SPU::power(bool reset) -> void {
  Thread::reset();
}

}
