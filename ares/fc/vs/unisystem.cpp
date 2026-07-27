#include <fc/fc.hpp>

namespace ares::Famicom {

VsUniSystem vsUniSystem;

#include "standard-input.cpp"
#include "zapper-input.cpp"
#include "controls.cpp"
#include "dip-switches.cpp"
#include "io.cpp"

auto VsUniSystem::load(Node::Object parent) -> void {
  this->parent = parent;
  node = parent->append<Node::Object>("Arcade");
  controls.load(node);
}

auto VsUniSystem::unload() -> void {
  disconnect();
  controls.unload();
  node.reset();
  parent.reset();
}

auto VsUniSystem::connect() -> bool {
  disconnect();
  if(!controls.connect()) return false;
  if(!dipSwitches.connect()) {
    disconnect();
    return false;
  }
  return true;
}

auto VsUniSystem::disconnect() -> void {
  controls.disconnect();
  dipSwitches.disconnect();
  io.power();
}

auto VsUniSystem::power() -> void {
  io.power();
}

auto VsUniSystem::frame() -> void {
  io.frame();
}

}
