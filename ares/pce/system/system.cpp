#include <pce/pce.hpp>
#include <algorithm>

namespace ares::PCEngine {

auto enumerate() -> std::vector<string> {
  return {
    "[NEC] PC Engine (NTSC-J)",
    "[NEC] TurboGrafx 16 (NTSC-U)",
    "[NEC] PC Engine Duo (NTSC-J)",
    "[NEC] TurboDuo (NTSC-U)",
    "[NEC] SuperGrafx (NTSC-J)",
    "[Pioneer] LaserActive (NEC PAC) (NTSC-U)",
    "[Pioneer] LaserActive (NEC PAC) (NTSC-J)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

auto option(string name, string value) -> bool {
  if(name == "Pixel Accuracy") vdp.setAccurate(true); // Forced: scanline renderer is too buggy
  return true;
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
  if(name.find("LaserActive")) {
    information.name = "PC Engine";
    information.model = Model::LaserActive;
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
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
