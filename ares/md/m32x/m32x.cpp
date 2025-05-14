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
  vectors.reset();
  sdram.reset();
  node.reset();
}

auto M32X::save() -> void {
}

auto M32X::power(bool reset) -> void {
  if constexpr(SH2::Accuracy::Recompiler) {
    if(!reset) ares::Memory::FixedAllocator::get().release();
  }
  sdram.fill(0);
  shm.power(reset);
  shs.power(reset);
  vdp.power(reset);
  pwm.power(reset);
  n32 vec4 = io.vectorLevel4;
  io = {};
  if(reset) io.vectorLevel4 = vec4;
  dreq = {};
  for(auto& word : communication) word = 0;

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
  if(vdp.hblank > line) {
    // TODO: VDP regs should be latched 192 MClks (~82 cycles) before end of hblank (according to official docs)
    vdp.latch.mode = vdp.mode;
    vdp.latch.lines = vdp.lines;
    vdp.latch.priority = vdp.priority;
    vdp.latch.dotshift = vdp.dotshift;
  }
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

auto M32X::debugRead(u32 address) -> u32 {
  /* if(address >= 0x0000'0000 && address <= 0x0000'3fff) {
    return bootROM[address >> 1];
  } */

  /* if(address >= 0x0000'4000 && address <= 0x0000'43ff) {
    return readInternalIO(upper, lower, address, data);
  } */

  /* if(address >= 0x0200'0000 && address <= 0x03ff'ffff) {
    return rom[address >> 1 & 0x1ff'ffff];
  } */

  if(address >= 0x0400'0000 && address <= 0x05ff'ffff) {
    return vdp.bbram[address >> 1 & 0xffff];
  }

  if(address >= 0x0600'0000 && address <= 0x0603'ffff) {
    return sdram[address >> 1 & 0x1ffff];
  }

  return 0;
}

}
