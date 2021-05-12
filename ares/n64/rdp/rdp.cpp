#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDP rdp;
#include "render.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RDP");
  debugger.load(node);
}

auto RDP::unload() -> void {
  debugger = {};
  node.reset();
}

auto RDP::main() -> void {
  step(system.frequency());
}

auto RDP::step(u32 clocks) -> void {
  Thread::clock += clocks;
}

auto RDP::power(bool reset) -> void {
  Thread::reset();
  command = {};
  edge = {};
  shade = {};
  texture = {};
  zbuffer = {};
  rectangle = {};
  other = {};
  fog = {};
  blend = {};
  primitive = {};
  environment = {};
  combine = {};
  tlut = {};
  load_ = {};
  tileSize = {};
  tile = {};
  set = {};
  primitiveDepth = {};
  scissor = {};
  convert = {};
  key = {};
  fillRectangle_ = {};
  io.bist = {};
  io.test = {};
}

}
