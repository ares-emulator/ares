#include <n64/n64.hpp>

#include <nall/gdb/server.hpp>

namespace ares::Nintendo64 {

auto enumerate() -> vector<string> {
  return {
    "[Nintendo] Nintendo 64 (NTSC)",
    "[Nintendo] Nintendo 64 (PAL)",
    "[Nintendo] Nintendo 64DD (NTSC-U)",
    "[Nintendo] Nintendo 64DD (NTSC-J)",
    "[Nintendo] Nintendo 64DD (NTSC-DEV)",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

auto option(string name, string value) -> bool {
  #if defined(VULKAN)
  if(name == "Enable GPU acceleration") vulkan.enable = value.boolean();
  if(name == "Quality" && value == "SD" ) vulkan.internalUpscale = 1;
  if(name == "Quality" && value == "HD" ) vulkan.internalUpscale = 2;
  if(name == "Quality" && value == "UHD") vulkan.internalUpscale = 4;
  if(name == "Supersampling") vulkan.supersampleScanout = value.boolean();
  if(name == "Disable Video Interface Processing") vulkan.disableVideoInterfaceProcessing = value.boolean();
  if(name == "Weave Deinterlacing") vulkan.weaveDeinterlacing = value.boolean();
  if(vulkan.internalUpscale == 1) vulkan.supersampleScanout = false;
  vulkan.outputUpscale = vulkan.supersampleScanout ? 1 : vulkan.internalUpscale;
  #endif
  return true;
}

System system;
Queue queue;
#include "serialization.cpp"

auto System::game() -> string {
  if(dd.node && !cartridge.node) {
    return dd.title();
  }

  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  cpu.main();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.match("[Nintendo] Nintendo 64 (*)")) {
    information.name = "Nintendo 64";
    information.dd = 0;
  }
  if(name.match("[Nintendo] Nintendo 64DD (*)")) {
    information.name = "Nintendo 64";
    information.dd = 1;
  }

  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.videoFrequency = 48'681'812;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.videoFrequency = 49'656'530;
  }

  node = Node::System::create(information.name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  controllerPort3.load(node);
  controllerPort4.load(node);
  rdram.load(node);
  mi.load(node);
  vi.load(node);
  ai.load(node);
  pi.load(node);
  pif.load(node);
  ri.load(node);
  si.load(node);
  cpu.load(node);
  rsp.load(node);
  rdp.load(node);
  if(_DD()) dd.load(node);
  #if defined(VULKAN)
  vulkan.load(node);
  #endif

  initDebugHooks();

  return true;
}

auto System::initDebugHooks() -> void {

  // See: https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
  GDB::server.hooks.targetXML = []() -> string {
    return "<target version=\"1.0\">"
      "<architecture>mips:4000</architecture>"
    "</target>";
  };

  GDB::server.hooks.normalizeAddress = [](u64 address) -> u64 {
    return cpu.devirtualizeDebug(address);
  };

  GDB::server.hooks.read = [](u64 address, u32 byteCount) -> string {
    address |= 0xFFFFFFFF'00000000ull;

    string res{};
    res.resize(byteCount * 2);
    char* resPtr = res.begin();

    for(u32 i : range(byteCount)) {
      auto val = cpu.readDebug(address++);
      hexByte(resPtr, val);
      resPtr += 2;
    }

    return res;
  };

  GDB::server.hooks.write = [](u64 address, u32 unitSize, u64 value) {
    address |= 0xFFFFFFFF'00000000ull;
    switch(unitSize) {
      case Byte: cpu.write<Byte>(address, value, false); break;
      case Half: cpu.write<Half>(address, value, false); break;
      case Word: cpu.write<Word>(address, value, false); break;
      case Dual: cpu.write<Dual>(address, value, false); break;
    }
  };

  GDB::server.hooks.regRead = [](u32 regIdx) {
    if(regIdx < 32) {
      return hex(cpu.ipu.r[regIdx].u64, 16, '0');
    }

    switch (regIdx)
    {
      case 32: return hex(cpu.getControlRegister(12), 16, '0'); // COP0 status
      case 33: return hex(cpu.ipu.lo.u64, 16, '0');
      case 34: return hex(cpu.ipu.hi.u64, 16, '0');
      case 35: return hex(cpu.getControlRegister(8), 16, '0'); // COP0 badvaddr
      case 36: return hex(cpu.getControlRegister(13), 16, '0'); // COP0 cause
      case 37: { // PC
        auto pcOverride = GDB::server.getPcOverride();
        return hex(pcOverride ? pcOverride.get() : cpu.ipu.pc, 16, '0');
      }

      // case 38-69: -> FPU
      case 70: return hex(cpu.getControlRegisterFPU(31), 16, '0'); // FPU control
    }

    if(regIdx < (38 + 32)) {
      return hex(cpu.fpu.r[regIdx-38].u64, 16, '0');
    }

    return string{"0000000000000000"};
  };

  GDB::server.hooks.regWrite = [](u32 regIdx, u64 regValue) -> bool {
    if(regIdx == 0)return true;

    if(regIdx < 32) {
      cpu.ipu.r[regIdx].u64 = regValue;
      return true;
    }
 
    switch (regIdx)
    {
      case 32: return true; // COP0 status (ignore write)
      case 33: cpu.ipu.lo.u64 = regValue; return true;
      case 34: cpu.ipu.hi.u64 = regValue; return true;
      case 35: return true; // COP0 badvaddr (ignore write)
      case 36: return true; // COP0 cause (ignore write)
      case 37: { // PC
        if(!GDB::server.getPcOverride()) {
          cpu.ipu.pc = regValue;
        }
        return true;
      }

      // case 38-69: -> FPU
      case 70: return true; // FPU control (ignore)
    }

    if(regIdx < (38 + 32)) {
      cpu.fpu.r[regIdx-38].u64 = regValue;
      return true;
    }

    if(regIdx == 71)return true; // ignore, GDB wants this register even though it doesn't exist
    return false;
  };

  GDB::server.hooks.regReadGeneral = []() {
    string res{};
    for(auto i : range(71)) {
      res.append(GDB::server.hooks.regRead(i));
    }
    return res;
  };

  GDB::server.hooks.regWriteGeneral = [](const string &regData) {
    u32 regIdx{0};
    for(auto i=0; i<regData.size(); i+=16) {
      GDB::server.hooks.regWrite(regIdx, regData.slice(i, 16).hex());
      ++regIdx;
    }
  };

  if constexpr(Accuracy::CPU::Recompiler) {
    GDB::server.hooks.emuCacheInvalidate = [](u64 address) {
      cpu.recompiler.invalidate(address);
    };
  }
}

auto System::unload() -> void {
  if(!node) return;
  save();
  
  if(vi.screen) vi.screen->quit(); //stop video thread
  #if defined(VULKAN)
  vulkan.unload();
  #endif
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  controllerPort3.unload();
  controllerPort4.unload();
  rdram.unload();
  mi.unload();
  vi.unload();
  ai.unload();
  pi.unload();
  pif.unload();
  ri.unload();
  si.unload();
  cpu.unload();
  rsp.unload();
  rdp.unload();
  if(_DD()) dd.unload();
  pak.reset();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  controllerPort1.save();
  controllerPort2.save();
  controllerPort3.save();
  controllerPort4.save();
  if(_DD()) dd.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if constexpr(Accuracy::CPU::Recompiler || Accuracy::RSP::Recompiler) {
    ares::Memory::FixedAllocator::get().release();
  }
  queue.reset();
  cartridge.power(reset);
  rdram.power(reset);
  if(_DD()) dd.power(reset);
  mi.power(reset);
  vi.power(reset);
  ai.power(reset);
  pi.power(reset);
  pif.power(reset);
  cic.power(reset);
  ri.power(reset);
  si.power(reset);
  cpu.power(reset);
  rsp.power(reset);
  rdp.power(reset);
}

}
