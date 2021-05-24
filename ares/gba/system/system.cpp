#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

auto enumerate() -> vector<string> {
  return {
    "[Nintendo] Game Boy Advance",
    "[Nintendo] Game Boy Player",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
BIOS bios;
System system;
#include "bios.cpp"
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  if(GameBoyAdvance::Model::GameBoyPlayer()) player.frame();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Game Boy Advance")) {
    information.name = "Game Boy Advance";
    information.model = Model::GameBoyAdvance;
  }
  if(name.find("Game Boy Player")) {
    information.name = "Game Boy Player";
    information.model = Model::GameBoyPlayer;
  }

  node = Node::System::create(information.name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  scheduler.reset();
  controls.load(node);
  bios.load(node);
  cpu.load(node);
  ppu.load(node);
  apu.load(node);
  cartridgeSlot.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  bios.unload();
  cpu.unload();
  ppu.unload();
  apu.unload();
  cartridgeSlot.unload();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  bus.power();
  player.power();
  cpu.power();
  ppu.power();
  apu.power();
  cartridge.power();
  scheduler.power(cpu);
}

}
