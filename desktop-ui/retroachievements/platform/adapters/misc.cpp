#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

namespace {

using namespace RA::Platform;

#if defined(CORE_CV) || defined(CORE_WS)
auto appendPakFile(const std::shared_ptr<vfs::directory>& pak, const string& name, std::vector<u8>& out) -> bool {
  if(!pak) return false;
  auto fp = pak->read(name);
  if(!fp) return false;
  auto offset = out.size();
  out.resize(offset + fp->size());
  for(u32 index : range(fp->size())) out[offset + index] = fp->read();
  return true;
}
#endif

#if defined(CORE_CV)
struct ColecoVisionAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    refreshMemoryNodeCache(cache, &emu.root, {{"CPU RAM", &cache.cpuRamMemory}}, &changed);
    if(!changed && !cache.memoryMap.empty()) return;

    // rcheevos reserves the following addresses for SGM RAM, but ares does
    // not currently expose the SGM's separate 8 KiB and 24 KiB RAM banks.
    cache.memoryMap = compileMemoryMap({
      {0x0000, 0x03ff, ReadMode::Linear, 0x0400, &cache.cpuRamMemory},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"ColecoVision"});
  }

  auto consoleId(const Emulator&) const -> u32 override { return RC_CONSOLE_COLECOVISION; }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    return emu.game && appendPakFile(emu.game->pak, "program.rom", out);
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

#if defined(CORE_WS)
struct WonderSwanAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
    ares::Node::Debugger::Memory cartridgeRamMemory;
    ares::Node::Debugger::Memory cartridgeEepromMemory;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU RAM", &cache.cpuRamMemory},
      {"Cartridge RAM", &cache.cartridgeRamMemory},
      {"Cartridge EEPROM", &cache.cartridgeEepromMemory},
    }, &changed);
    if(!changed && !cache.memoryMap.empty()) return;

    auto saveMemory = cache.cartridgeRamMemory ? &cache.cartridgeRamMemory : &cache.cartridgeEepromMemory;
    cache.memoryMap = compileMemoryMap({
      {0x00000, 0x0ffff, ReadMode::Linear, 0x10000, &cache.cpuRamMemory},
      {0x10000, 0x8ffff, ReadMode::Linear, 0x80000, saveMemory},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"WonderSwan", "WonderSwan Color", "Pocket Challenge V2"});
  }

  auto consoleId(const Emulator&) const -> u32 override { return RC_CONSOLE_WONDERSWAN; }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game) return false;
    if(appendPakFile(emu.game->pak, "program.rom", out)) return true;
    // mia overlays saved flash data on program.flash; hash the source image.
    if(file::exists(emu.game->location)) {
      out = emu.game->read(emu.game->location);
      return !out.empty();
    }
    if(directory::exists(emu.game->location)) {
      out = file::read({emu.game->location, "program.flash"});
      return !out.empty();
    }
    return false;
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

auto registerMiscAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_CV)
  static ColecoVisionAdapter colecoVision;
  list.push_back(&colecoVision);
#endif
#if defined(CORE_WS)
  static WonderSwanAdapter wonderSwan;
  list.push_back(&wonderSwan);
#endif
}

}
