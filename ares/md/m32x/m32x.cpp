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
  
  initDebugHooks();
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

auto M32X::initDebugHooks() -> void {

  // See: https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
  GDB::server.hooks.targetXML = []() -> string {
    return "<target version=\"1.0\">"
      "<architecture>sh2</architecture>"
    "</target>";
  };
  
  GDB::server.hooks.normalizeAddress = [](u64 address) -> u64 {
    return address & 0x00FFFFFF;
  };
  
  GDB::server.hooks.read = [](u64 address, u32 byteCount) -> string {
    address = (s32)address;

    string res{};
    res.resize(byteCount * 2);
    char* resPtr = res.begin();

    for(u32 i : range(byteCount)) {
      auto val = m32x.shm.readByte(address++);
      hexByte(resPtr, val);
      resPtr += 2;
    }

    return res;
  };
  
  GDB::server.hooks.regRead = [this](u32 regIdx) {
    if(regIdx < 16) {
      return hex(shm.regs.R[regIdx], 8, '0');
    }

    switch (regIdx)
    {
      case 16: { // PC
        auto pcOverride = GDB::server.getPcOverride();
        return hex(pcOverride ? pcOverride.get() : shm.regs.PC, 8, '0');
      }
      case 17: return hex(shm.regs.PR, 8, '0');
      case 18: return hex(shm.regs.GBR, 8, '0');
      case 19: return hex(shm.regs.MACL, 8, '0');
      case 20: return hex(shm.regs.MACH, 8, '0');
      // case 21: return hex((u32)shm.regs.SR, 8, '0');
    }

    return string{"00000000"};
  };
  
  GDB::server.hooks.regReadGeneral = []() {
    string res{};
    for(auto i : range(21)) {
      res.append(GDB::server.hooks.regRead(i));
    }
    return res;
  };
  
  if constexpr(SH2::Accuracy::Recompiler) {
    GDB::server.hooks.emuCacheInvalidate = [](u64 address) {
      m32x.shm.recompiler.invalidate(address, 4);
    };
  }
}

}
