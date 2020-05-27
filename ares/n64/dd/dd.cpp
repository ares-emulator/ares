#include <n64/n64.hpp>

namespace ares::Nintendo64 {

DD dd;
#include "io.cpp"
#include "serialization.cpp"

auto DD::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Disk Drive");
  c2s.allocate(0x400);
  ds.allocate(0x100);
  ms.allocate(0x40);
}

auto DD::unload() -> void {
  c2s.reset();
  ds.reset();
  ms.reset();
  node.reset();
  iplrom.reset();
}

auto DD::power(bool reset) -> void {
  iplrom.allocate(4_MiB);
  if(auto fp = platform->open(node, "64dd.ipl.rom", File::Read)) {
    iplrom.load(fp);
  }
}

}
