#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Timer timer;
#include "io.cpp"
#include "serialization.cpp"

auto Timer::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Timer");
}

auto Timer::unload() -> void {
  node.reset();
}

auto Timer::step(uint clocks) -> void {
}

auto Timer::power(bool reset) -> void {
}

}
