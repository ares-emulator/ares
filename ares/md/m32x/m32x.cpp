#include <md/md.hpp>

namespace ares::MegaDrive {

M32X m32x;
#include "sh7604.cpp"
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
  sdram.allocate(256_KiB >> 1);
  shm.load(node, "SHM", "sh2.boot.mrom");
  shs.load(node, "SHS", "sh2.boot.srom");
  vdp.load(node);
  pwm.load(node);
  debugger.load(node);

  if(auto fp = system.pak->read("vector.rom")) {
    vectors.allocate(fp->size() >> 1);
    for(auto address : range(vectors.size())) vectors.program(address, fp->readm(2L));
  }
}

auto M32X::unload() -> void {
  debugger = {};
  shm.unload();
  shs.unload();
  vdp.unload();
  pwm.unload(node);
  rom.reset();
  vectors.reset();
  sdram.reset();
  node.reset();
}

auto M32X::save() -> void {
}

auto M32X::power(bool reset) -> void {
  if constexpr(SH2::Accuracy::Recompiler) {
    ares::Memory::FixedAllocator::get().release();
  }
  sdram.fill(0);
  shm.power(reset);
  shs.power(reset);
  vdp.power(reset);
  pwm.power(reset);
  io = {};
  dreq = {};
  for(auto& word : communication) word = 0;

  io.vectorLevel4.byte(3) = vectors[0x70 >> 1].byte(1);
  io.vectorLevel4.byte(2) = vectors[0x70 >> 1].byte(0);
  io.vectorLevel4.byte(1) = vectors[0x72 >> 1].byte(1);
  io.vectorLevel4.byte(0) = vectors[0x72 >> 1].byte(0);

  //connect interfaces
  shm.sci.link = shs;
  shs.sci.link = shm;
  shm.debugger.self = shm;
  shs.debugger.self = shs;
  vdp.self = *this;
}

auto M32X::vblank(bool line) -> void {
  vdp.vblank = line;
  shm.irq.vint.active = line;
  shs.irq.vint.active = line;
  if(line) vdp.selectFramebuffer(vdp.framebufferSelect);
}

auto M32X::hblank(bool line) -> void {
  vdp.hblank = line;
  shm.irq.hint.active = 0;
  shs.irq.hint.active = 0;
  if(line && (!vdp.vblank || io.hintVblank)) {
    if(io.hcounter++ >= io.hperiod) {
      io.hcounter = 0;
      shm.irq.hint.active = 1;
      shs.irq.hint.active = 1;
    }
  }
}

}
