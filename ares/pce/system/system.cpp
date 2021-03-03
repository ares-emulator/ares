#include <pce/pce.hpp>

namespace ares::PCEngine {

auto enumerate() -> vector<string> {
  return {
    "[NEC] PC Engine (NTSC-J)",
    "[NEC] TurboGrafx 16 (NTSC-U)",
    "[NEC] PC Engine Duo (NTSC-J)",
    "[NEC] TurboDuo (NTSC-U)",
    "[NEC] SuperGrafx (NTSC-J)",
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
  if(pcd.disc) {
    return pcd.title();
  }

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
  if(name.find("PC Engine")) {
    information.name = "PC Engine";
    information.model = Model::PCEngine;
  }
  if(name.find("TurboGrafx 16")) {
    information.name = "PC Engine";
    information.model = Model::PCEngine;
  }
  if(name.find("PC Engine Duo")) {
    information.name = "PC Engine";
    information.model = Model::PCEngineDuo;
  }
  if(name.find("TurboDuo")) {
    information.name = "PC Engine";
    information.model = Model::PCEngineDuo;
  }
  if(name.find("SuperGrafx")) {
    information.name = "SuperGrafx";
    information.model = Model::SuperGrafx;
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
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
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  controllerPort.load(node);
  if(PCD::Present()) pcd.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  if(PCD::Present()) pcd.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  vdp.unload();
  psg.unload();
  cartridgeSlot.unload();
  controllerPort.unload();
  if(PCD::Present()) pcd.unload();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if(PCD::Present()) pcd.power();
  cartridgeSlot.power();
  cpu.power();
  vdp.power();
  psg.power();
  scheduler.power(cpu);
}

}
