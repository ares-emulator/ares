#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

namespace {

using namespace RA::Platform;

#if defined(CORE_NGP)
struct NeoGeoPocketAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
    ares::Node::Debugger::Memory apuRamMemory;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU RAM", &cache.cpuRamMemory},
      {"APU RAM", &cache.apuRamMemory},
    }, &changed);
    if(!changed && !cache.memoryMap.empty()) return;

    // rcheevos maps the CPU's 0x4000-0x7fff RAM window to 0x0000-0x3fff.
    cache.memoryMap = compileMemoryMap({
      {0x0000, 0x2fff, ReadMode::Linear, 0x3000, &cache.cpuRamMemory},
      {0x3000, 0x3fff, ReadMode::Linear, 0x1000, &cache.apuRamMemory},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Neo Geo Pocket", "Neo Geo Pocket Color"});
  }

  auto consoleId(const Emulator&) const -> u32 override { return RC_CONSOLE_NEOGEO_POCKET; }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game) return false;
    // mia pads program.flash and overlays saved flash data in its pak.
    if(file::exists(emu.game->location)) {
      out = emu.game->read(emu.game->location);
    } else if(directory::exists(emu.game->location)) {
      out = file::read({emu.game->location, "program.flash"});
    }
    return !out.empty();
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

#if defined(CORE_NG)
struct NeoGeoAesAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory workRamMemory;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    refreshMemoryNodeCache(cache, &emu.root, {{"System Work RAM", &cache.workRamMemory}}, &changed);
    if(!changed && !cache.memoryMap.empty()) return;

    cache.memoryMap = compileMemoryMap({
      {0x0000, 0xffff, ReadMode::LinearSwapped, 0x10000, &cache.workRamMemory},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Neo Geo AES"});
  }

  auto consoleId(const Emulator&) const -> u32 override { return RC_CONSOLE_ARCADE; }

  auto romData(const Emulator&, std::vector<u8>& out) const -> bool override {
    // Arcade ZIPs are identified by their set name, not assembled P-ROM bytes.
    out.clear();
    return false;
  }

  auto filePath(const Emulator& emu) const -> string override {
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

auto registerSnkAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_NGP)
  static NeoGeoPocketAdapter neoGeoPocket;
  list.push_back(&neoGeoPocket);
#endif
#if defined(CORE_NG)
  static NeoGeoAesAdapter neoGeoAes;
  list.push_back(&neoGeoAes);
#endif
}

}
