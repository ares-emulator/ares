#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

#if defined(CORE_GB)
#include <gb/gb.hpp>
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

#if defined(CORE_GB)
struct GameBoyAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory vram;
    ares::Node::Debugger::Memory wram;
    ares::Node::Debugger::Memory oam;
    ares::Node::Debugger::Memory hram;
    bool ready = false;
  };

  mutable Cache cache;

  auto refreshMemory(const Emulator& emu) const -> bool {
    ares::Node::Object root = emu.root;
    if(cache.rootNode != root) cache = {};
    if(cache.ready) return true;
    if(!refreshMemoryNodeCache(cache, &emu.root, {
      {"PPU VRAM", &cache.vram},
      {"CPU WRAM", &cache.wram},
      {"PPU OAM", &cache.oam},
      {"CPU HRAM", &cache.hram},
    })) return false;
    return cache.ready = cache.vram && cache.wram && cache.oam && cache.hram;
  }

  auto mappedMemory(u32 address, u32& offset) const -> ares::Node::Debugger::Memory {
    // Keep the base RAM banks stable; rcheevos exposes the remaining banks
    // above the Game Boy address space rather than following mapper selection.
    if(address >= 0x008000 && address <= 0x009fff) { offset = address - 0x008000; return cache.vram; }
    if(address >= 0x00c000 && address <= 0x00dfff) { offset = address - 0x00c000; return cache.wram; }
    if(address >= 0x00e000 && address <= 0x00fdff) { offset = address - 0x00e000; return cache.wram; }
    if(address >= 0x00fe00 && address <= 0x00fe9f) { offset = address - 0x00fe00; return cache.oam; }
    if(address >= 0x00ff80 && address <= 0x00fffe) {
      offset = address - 0x00ff80;
      return cache.hram;
    }
    if(address >= 0x010000 && address <= 0x015fff) {
      offset = 0x2000 + address - 0x010000;
      return cache.wram;
    }
    return {};
  }

  static auto cartridgeRam(u32 address, u32& offset) -> ares::Memory::Writable<n8>* {
    if(address >= 0x00a000 && address <= 0x00bfff) offset = address - 0x00a000;
    else if(address >= 0x016000 && address <= 0x033fff) offset = 0x2000 + address - 0x016000;
    else return nullptr;
    auto& cartridge = ares::GameBoy::cartridge;
    return cartridge.board ? cartridge.board->saveRAM : nullptr;
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Game Boy", "Game Boy Color"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    return emu.name == "Game Boy Color" ? RC_CONSOLE_GAMEBOY_COLOR : RC_CONSOLE_GAMEBOY;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    return emu.game && appendPakFile(emu.game->pak, "program.rom", out);
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size || !refreshMemory(emu)) return 0;
    u32 read = 0;
    for(; read < size; read++) {
      auto current = address + read;
      if(current <= 0x007fff) {
        auto& cartridge = ares::GameBoy::cartridge;
        if(!cartridge.board) break;
        buffer[read] = cartridge.board->read((n16)current, 0xff);
        continue;
      }
      u32 offset = 0;
      if(auto ram = cartridgeRam(current, offset)) {
        if(offset >= ram->size()) break;
        buffer[read] = ram->read(offset);
        continue;
      }
      if(current >= 0x00ff00 && current <= 0x00ff7f) {
        buffer[read] = ares::GameBoy::cpu.readDebugger((n16)current);
        continue;
      }
      if(current == 0x00ffff) {
        buffer[read] = ares::GameBoy::cpu.status.interruptEnable;
        continue;
      }
      auto memory = mappedMemory(current, offset);
      if(!memory || offset >= memory->size()) break;
      buffer[read] = memory->read(offset);
    }
    return read;
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size || !refreshMemory(emu)) return 0;
    u32 written = 0;
    for(; written < size; written++) {
      auto current = address + written;
      if(current <= 0x007fff || (current >= 0x00ff00 && current <= 0x00ff7f) || current == 0x00ffff) break;
      u32 offset = 0;
      if(auto ram = cartridgeRam(current, offset)) {
        if(offset >= ram->size()) break;
        ram->write(offset, buffer[written]);
        continue;
      }
      auto memory = mappedMemory(current, offset);
      if(!memory || offset >= memory->size()) break;
      memory->write(offset, buffer[written]);
    }
    return written;
  }
};
#endif


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

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size || address > 0x1fff || !ensureCpuRamMemory(emu)) return 0;
    auto writable = min(size, 0x2000 - address);
    for(u32 index : range(writable)) cache.cpuRamMemory->write((address + index) & 0x07ff, buffer[index]);
    return writable;
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

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(!refreshMemoryNodeCache(cache, &emu.root, {{"RDRAM", &cache.rdramMemory}}) || !cache.rdramMemory) return 0;
    auto rdramSize = cache.rdramMemory->size();
    if(address >= rdramSize) return 0;
    auto writable = min(size, rdramSize - address);
    for(u32 index : range(writable)) cache.rdramMemory->write((address + index) ^ 3, buffer[index]);
    return writable;
  }
};
#endif

}

namespace RA::Platform {

auto registerNintendoAdapters(std::vector<Adapter*>& list) -> void {
#if defined(CORE_GB)
  static GameBoyAdapter gameBoy;
  list.push_back(&gameBoy);
#endif
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
