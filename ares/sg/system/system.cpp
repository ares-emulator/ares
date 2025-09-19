#include <sg/sg.hpp>
#include <algorithm>

namespace ares::SG1000 {

auto enumerate() -> std::vector<string> {
  return {
    "[Sega] SG-1000 (NTSC)",
    "[Sega] SG-1000 (PAL)",
    "[Sega] SC-3000 (NTSC)",
    "[Sega] SC-3000 (PAL)",
    "[Sega] SG-1000A",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#include "arcade-controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("SG-1000")) {
    information.name = "SG-1000";
    information.model = Model::SG1000;
  }
  if(name.find("SG-1000A")) {
    information.name = "Arcade";
    information.model = Model::SG1000A;
  }
  if(name.find("SC-3000")) {
    information.name = "SC-3000";
    information.model = Model::SC3000;
  }
  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.colorburst = Constants::Colorburst::NTSC;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.colorburst = Constants::Colorburst::PAL * 4.0 / 5.0;
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

  scheduler.reset();
  if(information.model == Model::SG1000A) arcadeControls.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  ppi.load(node);
  cartridgeSlot.load(node);
  if(information.model != Model::SG1000A) {
    controllerPort1.load(node);
    controllerPort2.load(node);
  }
  if(information.model == Model::SC3000) {
    keyboard.load(node);
  }
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  vdp.unload();
  psg.unload();
  ppi.unload();
  cartridgeSlot.unload();
  if(information.model != Model::SG1000A) {
    controllerPort1.unload();
    controllerPort2.unload();
  }
  if(information.model == Model::SC3000) {
    keyboard.unload();
  }
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  cartridge.power();
  cpu.power();
  vdp.power();
  psg.power();
  ppi.power();
  scheduler.power(cpu);
}

}
