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
    "[SETA] Aleck 64",
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
  if(name == "Homebrew Mode") system.homebrewMode = value.boolean();
  if(name == "Recompiler") {
    if constexpr(Accuracy::CPU::Recompiler) {
      cpu.recompiler.enabled = value.boolean();
    }
    if constexpr(Accuracy::RSP::Recompiler) {
      rsp.recompiler.enabled = value.boolean();
    }
  }
  if(Model::Nintendo64() && name == "Expansion Pak") system.expansionPak = value.boolean();
  if(Model::Nintendo64() && name == "Controller Pak Banks") {
    if (value == "32KiB (Default)") {
      system.configuredControllerPakBankCount = 1;
      system.controllerPakBankCount = 1;
    } else if (value == "128KiB (Datel 1Meg)") {
      system.configuredControllerPakBankCount = 4;
      system.controllerPakBankCount = 4;
    } else if (value == "512KiB (Datel 4Meg)") {
      system.configuredControllerPakBankCount = 16;
      system.controllerPakBankCount = 16;
    } else if (value == "1984KiB (Maximum)") {
      system.configuredControllerPakBankCount = 62;
      system.controllerPakBankCount = 62;
    }
  }  
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
  if(_vulkanNeedsLoad) {
    vulkan.load(node);
    _vulkanNeedsLoad = false;
  }
  cpu.main();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  information.name = "Nintendo 64";

  if(name == "[SETA] Aleck 64") {
    information.name = "Arcade";
    information.model = Model::Aleck64;
    information.region = Region::NTSC;
    information.videoFrequency = 48'681'818;
    system.expansionPak = true; //Aleck 64 has the 8MB as standard
  } else {
    information.dd = name.find("64DD") ? true : false;
  }

  if (name.find("NTSC")) {
    information.region = Region::NTSC;
    information.videoFrequency = 48'681'818;
  }
  if (name.find("PAL")) {
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
  if(model() == Model::Aleck64) aleck64.load(node);

  initDebugHooks();
  _vulkanNeedsLoad = true;

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
    address = (s32)address;
    return cpu.devirtualizeDebug(address);
  };

  GDB::server.hooks.read = [](u64 address, u32 byteCount) -> string {
    address = (s32)address;

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

  GDB::server.hooks.write = [](u64 address, vector<u8> data) {
    address = (s32)address;

    // Handle writes of different/unaligned sizes only within the RDRAM area,
    // where we are sure that the write size does not really matter
    if(address >= 0xffff'ffff'8000'0000ull && address <= 0xffff'ffff'83ef'ffffull) {
      for(auto b : data) {
        cpu.dcache.writeDebug(address, address & 0x1fff'ffff, b);
        address++;
      }
    } else if (address >= 0xffff'ffff'a000'0000ull && address <= 0xffff'ffff'a3ef'ffffull) {
      Thread dummyThread{};
      for(auto b : data) {
        bus.write<Byte>(address & 0x1fff'ffff, b, dummyThread, "Ares Debugger");
        address++;
      }
    } else {
      // Otherwise, the write is expected to be of a set size to an aligned address.
      u64 value;
      switch(data.size()) {
        case Byte: 
          value = (u64)data[0];
          cpu.writeDebug<Byte>(address, value);
          break;
        case Half: 
          value = ((u64)data[0]<<8) | ((u64)data[1]<<0);
          cpu.writeDebug<Half>(address, value);
          break;
        case Word: 
          value = ((u64)data[0]<<24) | ((u64)data[1]<<16) | ((u64)data[2]<<8) | ((u64)data[3]<<0);
          cpu.writeDebug<Word>(address, value);
          break;
        case Dual:
          value  = ((u64)data[0]<<56) | ((u64)data[1]<<48) | ((u64)data[2]<<40) | ((u64)data[3]<<32);
          value |= ((u64)data[4]<<24) | ((u64)data[5]<<16) | ((u64)data[6]<< 8) | ((u64)data[7]<< 0);
          cpu.writeDebug<Dual>(address, value);
          break;
      }
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
          cpu.pipeline.setPc(regValue);
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
  _vulkanNeedsLoad = false;
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
  if(model() == Model::Aleck64) aleck64.unload();
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

  if(model() == Model::Aleck64) aleck64.save();
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
  if(model() == Model::Aleck64) aleck64.power(reset);
}

}
