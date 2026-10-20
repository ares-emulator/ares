#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

#if defined(CORE_SG)
  #include <sg/sg.hpp>
#endif

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

struct Sega8BitAdapter final : Adapter {
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
      {0x0000, 0x1fff, ReadMode::Linear, 0x2000, &cache.cpuRamMemory},
      {0x2000, 0x9fff, ReadMode::Linear, 0x8000, &cache.cartridgeRamMemory},
    });
  }

  static auto sgAddress(u32 address) -> u16 {
    if(address < 0x2000) return 0xc000 + address;
    if(address < 0x4000) return address;
    return 0x8000 + (address - 0x4000);
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"SG-1000", "Master System", "Game Gear"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    if(emu.name == "SG-1000") return RC_CONSOLE_SG1000;
    if(emu.name == "Game Gear") return RC_CONSOLE_GAME_GEAR;
    return RC_CONSOLE_MASTER_SYSTEM;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game || !emu.game->pak) return false;
    return appendPakFile(emu.game->pak, "program.rom", out);
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(emu.name == "SG-1000") {
#if defined(CORE_SG)
      if(!buffer || address >= 0x6000) return 0;
      auto readable = min(size, 0x6000 - address);
      // SG-1000 expansions appear at different CPU addresses; the cartridge
      // board resolves which regions actually contain RAM.
      for(u32 index : range(readable)) buffer[index] = ares::SG1000::cpu.read(sgAddress(address + index));
      return readable;
#else
      return 0;
#endif
    }

    refreshMemoryMap(emu);
    return Adapter::readMemoryMap(cache.memoryMap, address, buffer, size);
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(emu.name == "SG-1000") {
#if defined(CORE_SG)
      if(!buffer || address >= 0x6000) return 0;
      auto writable = min(size, 0x6000 - address);
      for(u32 index : range(writable)) ares::SG1000::cpu.write(sgAddress(address + index), buffer[index]);
      return writable;
#else
      return 0;
#endif
    }

    refreshMemoryMap(emu);
    return Adapter::writeMemoryMap(cache.memoryMap, address, buffer, size);
  }
};

struct MegaDriveAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
    ares::Node::Debugger::Memory m32xSdramMemory;
    ares::Node::Debugger::Memory cartridgeRamMemory;
    CompiledMap memoryMap;
    CompiledMap m32xMemoryMap;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    if(emu.name == "Mega 32X") {
      refreshMemoryNodeCache(cache, &emu.root, {
        {"CPU RAM", &cache.cpuRamMemory},
        {"32X SDRAM", &cache.m32xSdramMemory},
        {"Cartridge RAM", &cache.cartridgeRamMemory},
      }, &changed);
      if(!changed && !cache.m32xMemoryMap.empty()) return;
      cache.m32xMemoryMap = compileMemoryMap({
        {0x000000, 0x00ffff, ReadMode::LinearSwapped, 0x10000, &cache.cpuRamMemory},
        {0x010000, 0x04ffff, ReadMode::LinearSwapped, 0x40000, &cache.m32xSdramMemory},
        {0x050000, 0x05ffff, ReadMode::WrappedSwapped, 0, &cache.cartridgeRamMemory},
      });
      return;
    }

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
    return supportsAnyName(emu, {"Mega Drive", "Mega 32X"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    return emu.name == "Mega 32X" ? RC_CONSOLE_SEGA_32X : RC_CONSOLE_MEGA_DRIVE;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game || !emu.game->pak) return false;
    return appendPakFile(emu.game->pak, "program.rom", out);
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    refreshMemoryMap(emu);
    // rcheevos uses the 68K byte lane view (addr^1 on little-endian hosts).
    auto& memoryMap = emu.name == "Mega 32X" ? cache.m32xMemoryMap : cache.memoryMap;
    return Adapter::readMemoryMap(memoryMap, address, buffer, size);
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    refreshMemoryMap(emu);
    auto& memoryMap = emu.name == "Mega 32X" ? cache.m32xMemoryMap : cache.memoryMap;
    return Adapter::writeMemoryMap(memoryMap, address, buffer, size);
  }
};

}

namespace RA::Platform {

auto registerSegaAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_SG) || defined(CORE_MS)
  static Sega8BitAdapter sega8Bit;
  list.push_back(&sega8Bit);
#endif
#if defined(CORE_MD)
  static MegaDriveAdapter megaDrive;
  list.push_back(&megaDrive);
#endif
}

}
