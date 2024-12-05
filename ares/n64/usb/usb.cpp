#include <n64/n64.hpp>

namespace ares::Nintendo64 {

USB usb0(0);
USB usb1(1);
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto USB::load(Node::Object parent) -> void {
  string name = {"USB", num()};
  node = parent->append<Node::Object>(name);

  bdt.allocate(512);

  debugger.load(node);
}

auto USB::unload() -> void {
  node.reset();
  bdt.reset();
}

auto USB::power(bool reset) -> void {
  io = {};
  io.otgStatus.id = 1;

  bdt.fill();
}

}
