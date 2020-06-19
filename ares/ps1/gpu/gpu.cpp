#include <ps1/ps1.hpp>

namespace ares::PlayStation {

GPU gpu;
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("GPU");
}

auto GPU::unload() -> void {
  node.reset();
}

auto GPU::main() -> void {
}

auto GPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto GPU::power(bool reset) -> void {
  Thread::reset();
}

}
