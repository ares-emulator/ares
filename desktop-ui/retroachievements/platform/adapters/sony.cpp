#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

namespace {

using namespace RA::Platform;

#if defined(CORE_PS1)
struct PlayStationAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRam;
    ares::Node::Debugger::Memory scratchpad;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    if(!refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU RAM", &cache.cpuRam},
      {"CPU Scratchpad", &cache.scratchpad},
    }, &changed)) return;
    if(!changed && !cache.memoryMap.empty()) return;

    cache.memoryMap = compileMemoryMap({
      {0x000000, 0x1fffff, ReadMode::Linear, 0x200000, &cache.cpuRam},
      {0x200000, 0x2003ff, ReadMode::Linear, 0x400, &cache.scratchpad},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"PlayStation"});
  }

  auto consoleId(const Emulator&) const -> u32 override {
    return RC_CONSOLE_PLAYSTATION;
  }

  auto romData(const Emulator&, std::vector<u8>& out) const -> bool override {
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

auto registerSonyAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_PS1)
  static PlayStationAdapter playStation;
  list.push_back(&playStation);
#endif
}

}
