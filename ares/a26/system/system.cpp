#include <a26/a26.hpp>
#include <algorithm>

namespace ares::Atari2600 {

auto enumerate() -> std::vector<string> {
  return {
    "[Atari] Atari 2600 (NTSC)",
    "[Atari] Atari 2600 (PAL)",
    "[Atari] Atari 2600 (SECAM)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

Random random;
Scheduler scheduler;
System system;
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
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("NTSC")) {
    information.name = "Atari 2600";
    information.region = Region::NTSC;
    information.frequency = 3579546;
  }
  if(name.find("PAL")) {
    information.name = "Atari 2600";
    information.region = Region::PAL;
    information.frequency = 3546894;
  }
  if(name.find("SECAM")) {
    information.name = "Atari 2600";
    information.region = Region::SECAM;
    information.frequency = 3546894;
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
  controls.load(node);
  riot.load(node);
  cpu.load(node);
  tia.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  riot.unload();
  cpu.unload();
  tia.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::Low);
  cartridge.power();
  riot.power(reset);
  cpu.power(reset);
  tia.power(reset);
  scheduler.power(cpu);
}

}
