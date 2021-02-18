#include <n64/n64.hpp>

namespace ares::Nintendo64 {

DD dd;
#include "io.cpp"
#include "serialization.cpp"

auto DD::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Disk Drive");
  iplrom.allocate(4_MiB);
  c2s.allocate(0x400);
  ds.allocate(0x100);
  ms.allocate(0x40);

  if(auto fp = platform->open(node, "64dd.ipl.rom", File::Read)) {
    iplrom.load(fp);
  }
}

auto DD::unload() -> void {
  c2s.reset();
  ds.reset();
  ms.reset();
  node.reset();
  iplrom.reset();
}

auto DD::power(bool reset) -> void {
  c2s.fill();
  ds.fill();
  ms.fill();
}

}
