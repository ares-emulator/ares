#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Bus bus;
MemoryControl memory;
MemoryExpansion expansion1{ 8, 16, 31};
MemoryExpansion expansion2{11, 35, 84};
MemoryExpansion expansion3{ 7,  7, 11};
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
  Memory::Interface::setWaitStates(2, 2, 2);

  memory.ram = {};
  memory.cache = {};
}

}
