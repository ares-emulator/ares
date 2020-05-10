#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "io.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  rom.allocate(0x7c0);
  ram.allocate(0x040);
}

auto PI::unload() -> void {
  rom.reset();
  ram.reset();
}

auto PI::power() -> void {
  ram.fill();
  io = {};
}

}
