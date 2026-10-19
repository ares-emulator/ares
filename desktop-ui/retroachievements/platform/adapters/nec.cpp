#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

namespace {

using namespace RA::Platform;

#if defined(CORE_PCE)
struct PCEngineAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRam;
    ares::Node::Debugger::Memory cdWram;
    ares::Node::Debugger::Memory cdSram;
    ares::Node::Debugger::Memory cdBram;
    ares::Node::Debugger::Memory huCardRam;
    bool huCardRamChecked = false;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    ares::Node::Object root = emu.root;
    if(cache.rootNode != root) cache = {};
    bool changed = false;
    if(emu.name == "PC Engine CD") {
      if(!refreshMemoryNodeCache(cache, &emu.root, {
        {"CPU RAM", &cache.cpuRam},
        {"CD WRAM", &cache.cdWram},
        {"CD SRAM", &cache.cdSram},
        {"CD BRAM", &cache.cdBram},
      }, &changed)) return;
    } else if(emu.name == "SuperGrafx CD") {
      if(!refreshMemoryNodeCache(cache, &emu.root, {
        {"CPU RAM", &cache.cpuRam},
        {"CD WRAM", &cache.cdWram},
        {"CD BRAM", &cache.cdBram},
        {"HuCard RAM", &cache.huCardRam},
      }, &changed)) return;
    } else {
      if(!refreshMemoryNodeCache(cache, &emu.root, {
        {"CPU RAM", &cache.cpuRam},
      }, &changed)) return;
    }

    if(emu.name == "PC Engine CD" && !cache.huCardRamChecked) {
      bool cardRamChanged = false;
      refreshMemoryNodeCache(cache, &emu.root, {{"HuCard RAM", &cache.huCardRam}}, &cardRamChanged);
      cache.huCardRamChecked = true;
      changed |= cardRamChanged;
    }
    if(!changed && !cache.memoryMap.empty()) return;

    if(emu.name == "PC Engine CD" || emu.name == "SuperGrafx CD") {
      // Card RAM overrides the Duo's SRAM on the 0x68-0x7f banks when present.
      auto* superSystemRam = cache.huCardRam ? &cache.huCardRam : &cache.cdSram;
      cache.memoryMap = compileMemoryMap({
        {0x000000, 0x001fff, ReadMode::Linear, 0x2000, &cache.cpuRam},
        {0x002000, 0x011fff, ReadMode::Linear, 0x10000, &cache.cdWram},
        {0x012000, 0x041fff, ReadMode::Linear, 0x30000, superSystemRam},
        {0x042000, 0x0427ff, ReadMode::Linear, 0x800, &cache.cdBram},
      });
    } else {
      cache.memoryMap = compileMemoryMap({
        {0x000000, 0x001fff, ReadMode::Linear, 0x2000, &cache.cpuRam},
      });
    }
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"PC Engine", "SuperGrafx", "PC Engine CD", "SuperGrafx CD"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    if(emu.name == "PC Engine CD" || emu.name == "SuperGrafx CD") return RC_CONSOLE_PC_ENGINE_CD;
    return RC_CONSOLE_PC_ENGINE;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(emu.name == "PC Engine CD" || emu.name == "SuperGrafx CD") return false;
    if(!emu.game || !emu.game->pak) return false;
    auto fp = emu.game->pak->read("program.rom");
    if(!fp) return false;
    out.resize(fp->size());
    for(u32 index : range(out.size())) out[index] = fp->read();
    return !out.empty();
  }

  auto filePath(const Emulator& emu) const -> string override {
    if(emu.name != "PC Engine CD" && emu.name != "SuperGrafx CD") return {};
    return emu.game ? emu.game->location : string{};
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    refreshMemoryMap(emu);
    return Adapter::readMemoryMap(cache.memoryMap, address, buffer, size);
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    refreshMemoryMap(emu);
    return Adapter::writeMemoryMap(cache.memoryMap, address, buffer, size);
  }
};
#endif

}

namespace RA::Platform {

auto registerNecAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_PCE)
  static PCEngineAdapter pcEngine;
  list.push_back(&pcEngine);
#endif
}

}
