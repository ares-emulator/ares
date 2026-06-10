#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

#if defined(ARES_ENABLE_RCHEEVOS)

namespace {

using namespace RA::Platform;

auto appendPakFile(const std::shared_ptr<vfs::directory>& pak, const string& name, std::vector<u8>& out) -> bool {
  if(!pak) return false;
  auto fp = pak->read(name);
  if(!fp) return false;
  auto offset = out.size();
  out.resize(offset + fp->size());
  for(u32 index : range(fp->size())) out[offset + index] = fp->read();
  return true;
}

struct MegaDriveAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
    ares::Node::Debugger::Memory cartridgeRamMemory;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU RAM", &cache.cpuRamMemory},
      {"Cartridge RAM", &cache.cartridgeRamMemory},
    }, &changed);
    if(!changed && !cache.memoryMap.empty()) return;

    cache.memoryMap = compileMemoryMap({
      {0x000000, 0x00ffff, ReadMode::LinearSwapped, 0x10000, &cache.cpuRamMemory},
      {0x010000, 0x01ffff, ReadMode::WrappedSwapped, 0, &cache.cartridgeRamMemory},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Mega Drive"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    (void)emu;
    return RC_CONSOLE_MEGA_DRIVE;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game || !emu.game->pak) return false;
    return appendPakFile(emu.game->pak, "program.rom", out);
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    refreshMemoryMap(emu);
    // rcheevos Mega Drive map:
    // 0x000000-0x00ffff -> MD RAM
    // 0x010000-0x01ffff -> Cartridge RAM
    // rcheevos expects the libretro-style 68K byte lane view (addr^1 on LE
    // hosts for MD RAM). Mirror that layout here so achievement conditions
    // evaluate against the expected bytes.
    return Adapter::readMemoryMap(cache.memoryMap, address, buffer, size);
  }
};

}

namespace RA::Platform {

auto registerSegaAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_MD)
  static MegaDriveAdapter megaDrive;
  list.push_back(&megaDrive);
#endif
}

}

#endif
