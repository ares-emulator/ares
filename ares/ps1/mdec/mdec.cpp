#include <ps1/ps1.hpp>

namespace ares::PlayStation {

MDEC mdec;
#include "tables.cpp"
#include "decoder.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto MDEC::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("MDEC");
}

auto MDEC::unload() -> void {
  node.reset();
}

auto MDEC::power(bool reset) -> void {
  Thread::reset();
  Memory::Interface::setWaitStates(3, 3, 2);
}

}
