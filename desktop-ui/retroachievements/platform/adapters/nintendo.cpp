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

struct FamicomAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory cpuRamMemory;
  };

  mutable Cache cache;

  auto ensureCpuRamMemory(const Emulator& emu) const -> bool {
    if(cache.cpuRamMemory) return true;
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

}

namespace RA::Platform {

auto registerNintendoAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_FC)
  static FamicomAdapter famicom;
  list.push_back(&famicom);
#endif
}

}

#endif
