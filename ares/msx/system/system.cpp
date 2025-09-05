#include <msx/msx.hpp>
#include <algorithm>

namespace ares::MSX {

auto enumerate() -> std::vector<string> {
  return {
    "[Microsoft] MSX (NTSC)",
    "[Microsoft] MSX (PAL)",
    "[Microsoft] MSX2 (NTSC)",
    "[Microsoft] MSX2 (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

Scheduler scheduler;
ROM rom;
System system;
#include "serialization.cpp"

auto System::game() -> string {
  string game = {};

  if(cartridge.node) {
    game.append(cartridge.title());
  }

  if(tapeDeck.tray.tape) {
    if(game) game.append(" + ");
    game.append(tapeDeck.tray.tape.title());
  }

  if(expansion.node) {
    if(game) game.append(" + ");
    game.append(expansion.title());
  }

  if(!game) game = "(no cartridge connected)";
  return game;
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("MSX")) {
    information.name = "MSX";
    information.model = Model::MSX;
  }
  if(name.find("MSX2")) {
    information.name = "MSX2";
    information.model = Model::MSX2;
  }
  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.colorburst = Constants::Colorburst::NTSC;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.colorburst = Constants::Colorburst::PAL;
  }

  node = Node::System::create(information.name);
  node->setAttribute("configuration", name);
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
  keyboard.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  expansionSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  tapeDeck.load(node);
  if(model() == Model::MSX2) rtc.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  if(cartridge.node) cartridge.save();
  if(model() == Model::MSX2) rtc.save();
  expansion.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  vdp.unload();
  psg.unload();
  cartridgeSlot.unload();
  expansionSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  tapeDeck.unload();
  if(model() == Model::MSX2) rtc.unload();
  pak.reset();
  node.reset();
  rom.bios.reset();
  rom.sub.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  rom.bios.allocate(32_KiB);
  if(auto fp = pak->read("bios.rom")) {
    rom.bios.load(fp);
  }

  if(model() == Model::MSX2) {
    rom.sub.allocate(16_KiB);
    if(auto fp = pak->read("sub.rom")) {
      rom.sub.load(fp);
    }
  }

  keyboard.power();
  cartridge.power();
  expansion.power();
  tapeDeck.power();
  cpu.power();
  vdp.power();
  psg.power();
  if(model() == Model::MSX2) rtc.power();
  scheduler.power(cpu);
}

}
