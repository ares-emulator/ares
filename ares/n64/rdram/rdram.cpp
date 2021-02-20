#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDRAM rdram;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RDRAM::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RDRAM");

  //4_MiB internal
  //4_MiB expansion pak
  ram.allocate(4_MiB + 4_MiB);

  debugger.load(node);
}

auto RDRAM::unload() -> void {
  debugger = {};
  ram.reset();
  node.reset();
}

auto RDRAM::power(bool reset) -> void {
  ram.fill();
  //hacks needed for expansion pak detection:
  ram.writeWord(0x318, ram.size);  //CIC-NUS-6102
  ram.writeWord(0x3f0, ram.size);  //CIC-NUS-6105
  io = {};
}

}
