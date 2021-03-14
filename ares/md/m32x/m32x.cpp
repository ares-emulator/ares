#include <md/md.hpp>

namespace ares::MegaDrive {

M32X m32x;
#include "shm.cpp"
#include "shs.cpp"
#include "bus-internal.cpp"
#include "bus-external.cpp"
#include "io-internal.cpp"
#include "io-external.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto M32X::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Mega 32X");
  dram.allocate(256_KiB >> 1);
  sdram.allocate(256_KiB >> 1);
  palette.allocate(512 >> 1);
  shm.load(node);
  shs.load(node);
}

auto M32X::unload() -> void {
  shm.unload();
  shs.unload();
  rom.reset();
  dram.reset();
  sdram.reset();
  palette.reset();
  node.reset();
}

auto M32X::save() -> void {
}

auto M32X::main() -> void {
}

auto M32X::power(bool reset) -> void {
  shm.power(reset);
  shs.power(reset);
  io = {};
  dreq = {};
  for(auto& word : communication) word = 0;
}

}
