#include <gba/gba.hpp>
#include <algorithm>

namespace ares::GameBoyAdvance {

auto enumerate() -> std::vector<string> {
  return {
    "[Nintendo] Game Boy Advance",
    "[Nintendo] Game Boy Player",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

auto option(string name, string value) -> bool {
  if(name == "Pixel Accuracy") {
    ppu.setAccurate(value.boolean());
  }
  return true;
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
    information.model = Model::GameBoyAdvance;
  }
  if(name.find("Game Boy Player")) {
    information.model = Model::GameBoyPlayer;
  }

  node = std::make_shared<Core::System>(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  node->setSerialize([this](bool save) -> serializer { return serialize(save); });
  node->setUnserialize(std::bind_front(&System::unserialize, this));
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  scheduler.reset();
  controls.load(node);
  bios.load(node);
  cpu.load(node);
  ppu.load(node);
  apu.load(node);
  display.load(node);
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
  display.unload();
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
  display.power();
  cartridge.power();
  scheduler.power(cpu);
}

}
