#include <fc/fc.hpp>

namespace ares::Famicom {

VsUniSystem vsUniSystem;

#include "standard-input.cpp"
#include "zapper-input.cpp"
#include "controls.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto VsUniSystem::load(Node::Object parent) -> void {
  this->parent = parent;
  node = parent->append<Node::Object>("Arcade");
}

auto VsUniSystem::unload() -> void {
  dips.unload();
  controls.unload();
  io.power();
  node.reset();
  parent.reset();
}

auto VsUniSystem::power(bool reset) -> void {
  io.power();
  if(reset) return;

  if(!controls.load(node)) return;
  dips.load(parent, cartridge.pak, {"DSW0"});
}

auto VsUniSystem::frame() -> void {
  controls.frame();
  io.frame();
}

}
