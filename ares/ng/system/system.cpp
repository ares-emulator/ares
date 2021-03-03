#include <ng/ng.hpp>

namespace ares::NeoGeo {

auto enumerate() -> vector<string> {
  return {
    "[SNK] Neo Geo AES",
    "[SNK] Neo Geo MVS",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
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
  if(name.find("Neo Geo AES")) {
    information.name = "Neo Geo AES";
    information.model = Model::NeoGeoAES;
  }
  if(name.find("Neo Geo MVS")) {
    information.name = "Neo Geo MVS";
    information.model = Model::NeoGeoMVS;
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

  scheduler.reset();
  cpu.load(node);
  apu.load(node);
  gpu.load(node);
  opnb.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  apu.unload();
  gpu.unload();
  opnb.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if(cartridge.node) cartridge.power();
  cpu.power(reset);
  apu.power(reset);
  gpu.power(reset);
  opnb.power(reset);
  scheduler.power(cpu);
}

};
