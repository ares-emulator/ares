#include <md/md.hpp>

namespace ares::MegaDrive {

M32X m32x;
#include "shm.cpp"
#include "shs.cpp"
#include "bus-internal.cpp"
#include "bus-external.cpp"
#include "io-internal.cpp"
#include "io-external.cpp"
#include "vdp.cpp"
#include "pwm.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto M32X::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Mega 32X");
  vectors.allocate(256 >> 1);
  dram.allocate(256_KiB >> 1);
  sdram.allocate(256_KiB >> 1);
  cram.allocate(512 >> 1);
  shm.load(node);
  shs.load(node);
  debugger.load(node);

  if(auto fp = system.pak->read("vector.rom")) {
    for(auto address : range(256 >> 1)) vectors.program(address, fp->readm(2L));
  }
}

auto M32X::unload() -> void {
  debugger = {};
  shm.unload();
  shs.unload();
  rom.reset();
  vectors.reset();
  dram.reset();
  sdram.reset();
  cram.reset();
  node.reset();
}

auto M32X::save() -> void {
}

auto M32X::main() -> void {
}

auto M32X::power(bool reset) -> void {
  dram.fill(0);
  sdram.fill(0);
  cram.fill(0);
  shm.power(reset);
  shs.power(reset);
  io = {};
  dreq = {};
  for(auto& word : communication) word = 0;

  io.vectorLevel4.byte(3) = vectors[0x70 >> 1].byte(1);
  io.vectorLevel4.byte(2) = vectors[0x70 >> 1].byte(0);
  io.vectorLevel4.byte(1) = vectors[0x72 >> 1].byte(1);
  io.vectorLevel4.byte(0) = vectors[0x72 >> 1].byte(0);
}

auto M32X::vblank(bool line) -> void {
  vdp.vblank = line;
  shm.irq.vint.active = line;
  shs.irq.vint.active = line;
}

auto M32X::hblank(bool line) -> void {
  vdp.hblank = line;
  if(io.hcounter++ >= io.htarget) {
    io.hcounter = 0;
    shm.irq.hint.active = line;
    shs.irq.hint.active = line;
  }
}

}
