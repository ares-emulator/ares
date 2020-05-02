#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDRAM rdram;
#include "io.cpp"
#include "serialization.cpp"

auto RDRAM::load(Node::Object parent) -> void {
  //4_MiB internal
  //4_MiB expansion pak
  ram.allocate(4_MiB + 4_MiB);
}

auto RDRAM::unload() -> void {
  ram.reset();
}

auto RDRAM::power() -> void {
  io = {};
}

}
