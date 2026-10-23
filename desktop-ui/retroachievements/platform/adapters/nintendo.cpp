#include "../../../desktop-ui.hpp"
#include "../adapter.hpp"

#include <rc_consoles.h>

#if defined(CORE_FC)
#include <fc/fc.hpp>
#endif
#if defined(CORE_GB)
#include <gb/gb.hpp>
#endif
#if defined(CORE_GBA)
#include <gba/gba.hpp>
#endif
#if defined(CORE_SFC)
#include <sfc/sfc.hpp>
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

#if defined(CORE_FC)
auto appendFdsSide(const string& location, const string& name, std::vector<u8>& out) -> bool {
  auto side = file::read({location, name});
  if(side.empty()) return false;
  if(side.size() == 65500) {
    out.insert(out.end(), side.begin(), side.end());
    return true;
  }
  if(side.size() < 0x12000) return false;

  // mia expands FDS blocks with gaps, sync bytes, and CRCs for the drive.
  // Recover the 65500-byte side image used by rcheevos for directory media.
  std::vector<u8> canonical;
  canonical.reserve(65500);
  size_t offset = 0;
  auto skip = [&](size_t count) -> bool {
    if(count > side.size() - offset) return false;
    offset += count;
    return true;
  };
  auto block = [&](size_t gap, size_t length) -> bool {
    if(!skip(gap) || offset >= side.size() || side[offset++] != 0x80) return false;
    if(length > side.size() - offset) return false;
    canonical.insert(canonical.end(), side.begin() + offset, side.begin() + offset + length);
    offset += length;
    return skip(2);
  };

  if(!block(0x0e00, 0x38) || !block(0x80, 2)) return false;
  while(offset + 0x80 + 1 + 0x10 + 2 + 0x80 + 1 <= side.size()) {
    auto header = offset + 0x80 + 1;
    if(side[header - 1] != 0x80 || side[header] != 0x03) break;
    auto dataSize = (u16)(side[header + 0x0d] | side[header + 0x0e] << 8);
    if(!block(0x80, 0x10) || !block(0x80, 1 + dataSize)) return false;
  }
  if(canonical.size() > 65500) return false;
  canonical.resize(65500, 0);
  out.insert(out.end(), canonical.begin(), canonical.end());
  return true;
}
#endif

#if defined(CORE_SFC)
auto readSfcRomFromManifest(const std::shared_ptr<vfs::directory>& pak, std::vector<u8>& out) -> bool {
  out.clear();
  if(!pak) return false;
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto document = BML::unserialize(fp->reads());
  if(!document) return false;

  bool found = false;
  for(auto node : document.find("game/board/memory(type=ROM)")) {
    string name;
    if(auto architecture = node["architecture"].string()) name.append(architecture.downcase(), ".");
    name.append(node["content"].string().downcase(), ".rom");
    if(!appendPakFile(pak, name, out)) {
      out.clear();
      return false;
    }
    found = true;
  }
  return found;
}
#endif

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

#if defined(CORE_GBA)
struct GameBoyAdvanceAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory iwram;
    ares::Node::Debugger::Memory ewram;
    CompiledMap memoryMap;
    ares::VFS::Pak savePak;
    std::span<n8> save;
  };

  mutable Cache cache;

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    if(!refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU IWRAM", &cache.iwram},
      {"CPU EWRAM", &cache.ewram},
    }, &changed)) return;
    if(!changed && !cache.memoryMap.empty()) return;

    cache.memoryMap = compileMemoryMap({
      {0x000000, 0x007fff, ReadMode::Linear, 0x8000, &cache.iwram},
      {0x008000, 0x047fff, ReadMode::Linear, 0x40000, &cache.ewram},
    });
  }

  auto saveMemory(const Emulator& emu) const -> std::span<n8> {
    if(!emu.game || !emu.game->pak) return {};
    auto& cartridge = ares::GameBoyAdvance::cartridge;
    if(cartridge.pak != emu.game->pak) return {};
    if(cache.savePak != cartridge.pak) {
      cache.savePak = cartridge.pak;
      cache.save = {};
      if(cartridge.pak->read("save.flash")) cache.save = {cartridge.flash.data, cartridge.flash.size};
      else if(cartridge.pak->read("save.ram")) cache.save = {cartridge.sram.data, cartridge.sram.size};
      else if(cartridge.pak->read("save.eeprom")) cache.save = {cartridge.eeprom.data, cartridge.eeprom.size};
    }
    return cache.save;
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Game Boy Advance"});
  }

  auto consoleId(const Emulator&) const -> u32 override {
    return RC_CONSOLE_GAMEBOY_ADVANCE;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    return emu.game && appendPakFile(emu.game->pak, "program.rom", out);
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(address >= 0x048000 && address <= 0x057fff) {
      auto save = saveMemory(emu);
      auto offset = address - 0x048000;
      if(offset >= save.size()) return 0;
      auto readable = min(size, min((u32)save.size() - offset, 0x058000 - address));
      for(u32 index : range(readable)) buffer[index] = save[offset + index];
      return readable;
    }
    refreshMemoryMap(emu);
    return Adapter::readMemoryMap(cache.memoryMap, address, buffer, size);
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(address >= 0x048000 && address <= 0x057fff) {
      auto save = saveMemory(emu);
      auto offset = address - 0x048000;
      if(offset >= save.size()) return 0;
      auto writable = min(size, min((u32)save.size() - offset, 0x058000 - address));
      for(u32 index : range(writable)) save[offset + index] = buffer[index];
      return writable;
    }
    refreshMemoryMap(emu);
    return Adapter::writeMemoryMap(cache.memoryMap, address, buffer, size);
  }
};
#endif

#if defined(CORE_SFC)
struct SuperFamicomAdapter final : Adapter {
  struct Cache : NodeCache {
    ares::Node::Debugger::Memory wram;
    ares::Node::Debugger::Memory cartridgeRam;
    ares::Node::Debugger::Memory sa1Bwram;
    ares::Node::Debugger::Memory sa1Iram;
    CompiledMap memoryMap;
  };

  mutable Cache cache;

  static auto sufamiRam(u32 address, u32& offset) -> ares::SuperFamicom::WritableMemory* {
    if(address < 0x020000 || address > 0x09ffff) return nullptr;
    auto& slotA = ares::SuperFamicom::sufamiturboA;
    auto& slotB = ares::SuperFamicom::sufamiturboB;
    offset = address - 0x020000;
    if(slotA.node && offset < slotA.ram.size()) return &slotA.ram;
    if(slotA.node) offset -= slotA.ram.size();
    if(slotB.node && offset < slotB.ram.size()) return &slotB.ram;
    return nullptr;
  }

  auto refreshMemoryMap(const Emulator& emu) const -> void {
    bool changed = false;
    if(!refreshMemoryNodeCache(cache, &emu.root, {
      {"CPU WRAM", &cache.wram},
      {"Cartridge RAM", &cache.cartridgeRam},
      {"SA1 BWRAM", &cache.sa1Bwram},
      {"SA1 IRAM", &cache.sa1Iram},
    }, &changed)) return;
    if(!changed && !cache.memoryMap.empty()) return;

    auto saveRam = cache.sa1Bwram ? &cache.sa1Bwram : &cache.cartridgeRam;
    cache.memoryMap = compileMemoryMap({
      {0x000000, 0x01ffff, ReadMode::Linear, 0x20000, &cache.wram},
      {0x020000, 0x09ffff, ReadMode::Linear, 0x80000, saveRam},
      {0x0a0000, 0x0a07ff, ReadMode::Linear, 0x800, &cache.sa1Iram},
    });
  }

  auto supports(const Emulator& emu) const -> bool override {
    return supportsAnyName(emu, {"Super Famicom"});
  }

  auto consoleId(const Emulator&) const -> u32 override {
    return RC_CONSOLE_SUPER_NINTENDO;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game) return false;

    // Hash the original cartridge image: mia can split its ROM and append
    // coprocessor firmware while preparing the pak for the core.
    if(file::exists(emu.game->location)) {
      out = emu.game->read(emu.game->location, {"*.sfc", "*.smc", "*.swc", "*.fig", "*.st"});
      if(!out.empty()) return true;
    }
    if(readSfcRomFromManifest(emu.game->pak, out)) return true;

    // An attached Sufami Turbo cartridge is a fallback when the base cart
    // does not provide a ROM to hash.
    if(emu.root) {
      std::vector<u8> slotA, slotB;
      for(auto node : ares::Node::enumerate<ares::Node::Peripheral>(emu.root)) {
        if(!node || node->name() != "Sufami Turbo Cartridge") continue;
        auto parent = node->parent().lock();
        if(!parent) continue;
        auto pak = const_cast<Emulator&>(emu).pak(node);
        if(parent->name() == "Sufami Turbo Slot A") readSfcRomFromManifest(pak, slotA);
        if(parent->name() == "Sufami Turbo Slot B") readSfcRomFromManifest(pak, slotB);
      }
      if(!slotA.empty() || !slotB.empty()) {
        out = std::move(slotA);
        out.insert(out.end(), slotB.begin(), slotB.end());
        return true;
      }
    }
    return false;
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(address >= 0x020000 && address <= 0x09ffff &&
       (ares::SuperFamicom::sufamiturboA.node || ares::SuperFamicom::sufamiturboB.node)) {
      u32 offset = 0;
      auto ram = sufamiRam(address, offset);
      if(!ram) return 0;
      auto readable = min(size, ram->size() - offset);
      for(u32 index : range(readable)) buffer[index] = ram->read(offset + index);
      return readable;
    }
    refreshMemoryMap(emu);
    return Adapter::readMemoryMap(cache.memoryMap, address, buffer, size);
  }

  auto writeMemory(const Emulator& emu, u32 address, const u8* buffer, u32 size) const -> u32 override {
    if(!buffer || !size) return 0;
    if(address >= 0x020000 && address <= 0x09ffff &&
       (ares::SuperFamicom::sufamiturboA.node || ares::SuperFamicom::sufamiturboB.node)) {
      u32 offset = 0;
      auto ram = sufamiRam(address, offset);
      if(!ram) return 0;
      auto writable = min(size, ram->size() - offset);
      for(u32 index : range(writable)) ram->write(offset + index, buffer[index]);
      return writable;
    }
    refreshMemoryMap(emu);
    return Adapter::writeMemoryMap(cache.memoryMap, address, buffer, size);
  }
};
#endif

#if defined(CORE_FC)
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
    return supportsAnyName(emu, {"Famicom", "Famicom Disk System"});
  }

  auto consoleId(const Emulator& emu) const -> u32 override {
    return emu.name == "Famicom Disk System" ? RC_CONSOLE_FAMICOM_DISK_SYSTEM : RC_CONSOLE_NINTENDO;
  }

  auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool override {
    out.clear();
    if(!emu.game || !emu.game->pak) return false;

    if(emu.name == "Famicom Disk System") {
      if(file::exists(emu.game->location)) {
        out = emu.game->read(emu.game->location);
        return !out.empty();
      }
      if(!directory::exists(emu.game->location)) return false;
      if(!appendFdsSide(emu.game->location, "disk1.sideA", out)) return false;
      for(auto name : {"disk1.sideB", "disk2.sideA", "disk2.sideB"}) {
        if(file::exists({emu.game->location, name}) && !appendFdsSide(emu.game->location, name, out)) {
          out.clear();
          return false;
        }
      }
      return true;
    }

    auto hasInes = appendPakFile(emu.game->pak, "ines.rom", out);
    auto hasProgramFlash = appendPakFile(emu.game->pak, "program.flash", out);
    auto hasProgram = appendPakFile(emu.game->pak, "program.rom", out);
    auto hasOption = appendPakFile(emu.game->pak, "option.rom", out);
    auto hasCharacter = appendPakFile(emu.game->pak, "character.rom", out);
    return hasInes || hasProgramFlash || hasProgram || hasOption || hasCharacter;
  }

  auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 override {
    if(!buffer || size == 0) return 0;
    if(emu.name == "Famicom Disk System" && address >= 0x6000 && address <= 0xdfff) {
      auto readable = min(size, 0xe000 - address);
      for(u32 index : range(readable)) buffer[index] = ares::Famicom::cartridge.readPRG(address + index, 0xff);
      return readable;
    }
    if(!ensureCpuRamMemory(emu)) return 0;
    // rcheevos RAM exposure for NES and FDS:
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
    if(!buffer || !size) return 0;
    if(emu.name == "Famicom Disk System" && address >= 0x6000 && address <= 0xdfff) {
      auto writable = min(size, 0xe000 - address);
      for(u32 index : range(writable)) ares::Famicom::cartridge.writePRG(address + index, buffer[index]);
      return writable;
    }
    if(address > 0x1fff || !ensureCpuRamMemory(emu)) return 0;
    auto writable = min(size, 0x2000 - address);
    for(u32 index : range(writable)) cache.cpuRamMemory->write((address + index) & 0x07ff, buffer[index]);
    return writable;
  }
};
#endif

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
#if defined(CORE_GBA)
  static GameBoyAdvanceAdapter gameBoyAdvance;
  list.push_back(&gameBoyAdvance);
#endif
#if defined(CORE_SFC)
  static SuperFamicomAdapter superFamicom;
  list.push_back(&superFamicom);
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
