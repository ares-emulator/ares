#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

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

struct FamicomAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
  };

  mutable Cache cache;

  auto ensureCpuRamMemory(const Emulator& emu) const -> bool {
    if(!refreshMemoryNodeCache(cache, &emu.root, {{"CPU RAM", &cache.cpuRamMemory}})) return false;
    return (bool)cache.cpuRamMemory;
  }

  auto supports(const Emulator& emu) const -> bool override {
    // FDS is excluded for now
    return supportsAnyName(emu, {"Famicom"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    (void)emu;
    return RC_CONSOLE_NINTENDO;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game || !emu.game->pak) return false;

    auto hasInes = appendPakFile(emu.game->pak, "ines.rom", out);
    auto hasProgramFlash = appendPakFile(emu.game->pak, "program.flash", out);
    auto hasProgram = appendPakFile(emu.game->pak, "program.rom", out);
    auto hasOption = appendPakFile(emu.game->pak, "option.rom", out);
    auto hasCharacter = appendPakFile(emu.game->pak, "character.rom", out);
    return hasInes || hasProgramFlash || hasProgram || hasOption || hasCharacter;
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || size == 0) return 0;
    if(!ensureCpuRamMemory(emu)) return 0;
    // rcheevos RAM exposure for NES:
    // 0x0000-0x07ff base RAM, mirrored to 0x1fff.
    if(address > 0x1fff) return 0;
    auto readable = size;
    if(address + readable > 0x2000) readable = 0x2000 - address;
    for(u32 index : range(readable)) {
      buffer[index] = cache.cpuRamMemory->read((address + index) & 0x07ff);
    }
    return readable;
  }
};

#if defined(CORE_N64)
struct Nintendo64Adapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory rdramMemory;
  };

  mutable Cache cache;

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Nintendo 64", "Nintendo 64DD"});
  }

  auto consoleId(const Emulator&) const -> u32 override {
    return RC_CONSOLE_NINTENDO_64;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game) return false;

    if(emu.name == "Nintendo 64") {
      // mia has already normalized .n64 and .v64 cartridges to .z64 byte order.
      return appendPakFile(emu.game->pak, "program.rom", out);
    }

    // 64DD hashes use the source disk image. mia converts .ndd/.d64 images to
    // its physical disk layout and may overlay saved data in program.disk.
    if(directory::exists(emu.game->location)) out = file::read({emu.game->location, "program.disk"});
    else out = emu.game->read(emu.game->location);
    return !out.empty();
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(!refreshMemoryNodeCache(cache, &emu.root, {{"RDRAM", &cache.rdramMemory}})) return 0;
    if(!cache.rdramMemory) return 0;

    // RA addresses RDRAM from zero, including the Expansion Pak when present.
    auto rdramSize = cache.rdramMemory->size();
    if(address >= rdramSize) return 0;
    auto readable = min(size, rdramSize - address);
    for(u32 index : range(readable)) {
      // Match the byte-swapped N64 RAM view used by other RA integrations.
      buffer[index] = cache.rdramMemory->read((address + index) ^ 3);
    }
    return readable;
  }
};
#endif

}

namespace RA::Platform {

auto registerNintendoAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_FC)
  static FamicomAdapter famicom;
  list.push_back(&famicom);
#endif
#if defined(CORE_N64)
  static Nintendo64Adapter nintendo64;
  list.push_back(&nintendo64);
#endif
}

}
