#include <n64/n64.hpp>

namespace ares::Nintendo64 {

DD dd;
#include "io.cpp"
#include "serialization.cpp"

auto DD::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Disk Drive");
  if(!node->setPak(pak = platform->pak(node))) return;

  iplrom.allocate(4_MiB);
  c2s.allocate(0x400);
  ds.allocate(0x100);
  ms.allocate(0x40);

  if(auto fp = pak->read("64dd.ipl.rom")) {
    iplrom.load(fp);
  }
}

auto DD::unload() -> void {
  iplrom.reset();
  c2s.reset();
  ds.reset();
  ms.reset();
  pak.reset();
  node.reset();
}

auto DD::power(bool reset) -> void {
  c2s.fill();
  ds.fill();
  ms.fill();
}

}
