#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

namespace {

using namespace RA::Platform;

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
#if defined(CORE_NG)
  static NeoGeoAesAdapter neoGeoAes;
  list.push_back(&neoGeoAes);
#endif
}

}
