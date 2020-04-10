#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Interface* interface = nullptr;

auto SuperFamicomInterface::game() -> string {
  #if defined(CORE_GB)
  if(cartridge.has.GameBoySlot && GameBoy::cartridge.node) {
    return GameBoy::cartridge.name();
  }
  #endif

  if(bsmemory.node) {
    return {cartridge.name(), " + ", bsmemory.name()};
  }

  if(sufamiturboA.node && sufamiturboB.node) {
    return {sufamiturboA.name(), " + ", sufamiturboB.name()};
  }

  if(sufamiturboA.node) {
    return sufamiturboA.name();
  }

  if(sufamiturboB.node) {
    return sufamiturboB.name();
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto SuperFamicomInterface::root() -> Node::Object {
  return system.node;
}

auto SuperFamicomInterface::load(Node::Object& root) -> void {
  interface = this;
  system.load(root);
}

auto SuperFamicomInterface::unload() -> void {
  system.unload();
}

auto SuperFamicomInterface::save() -> void {
  system.save();
}

auto SuperFamicomInterface::power() -> void {
  system.power(false);
}

auto SuperFamicomInterface::run() -> void {
  system.run();
}

auto SuperFamicomInterface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto SuperFamicomInterface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

auto SuperFamicomInterface::exportMemory() -> bool {
  directory::create("/tmpfs/Memory/Super Famicom/");
  file::write("/tmpfs/Memory/Super Famicom/wram.bin", {cpu.wram, 128 * 1024});
  file::write("/tmpfs/Memory/Super Famicom/vram.bin", {ppu.vram.data, 64 * 1024});
  return true;
}

}
