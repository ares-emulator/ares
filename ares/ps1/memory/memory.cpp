#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Bus bus;
MemoryControl memory;
MemoryExpansion expansion1{ 6, 13, 25};
MemoryExpansion expansion2{10, 25, 55};
MemoryExpansion expansion3{ 6,  5,  9};
Memory::Readable bios;
Memory::Unmapped unmapped;
#include "io.cpp"
#include "serialization.cpp"

auto MemoryControl::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Memory");
}

auto MemoryControl::unload() -> void {
  node.reset();
}

auto MemoryControl::power(bool reset) -> void {
  //timings are for $1f80:106x
  //timings for $fffe:xxxx are hardcoded as (0, 1, 1) in cpu/core/memory.cpp
  Memory::Interface::setWaitStates(2, 2, 2);

  memory.ram = {};
  memory.cache = {};
}

}
