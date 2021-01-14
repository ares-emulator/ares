#include <pce/pce.hpp>

namespace ares::PCEngine {

auto load(Node::System& node, string name) -> bool {
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#include "serialization.cpp"

auto System::game() -> string {
  if(pcd.disc) {
    return pcd.name();
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name == "PC Engine"    ) information.model = Model::PCEngine;
  if(name == "PC Engine Duo") information.model = Model::PCEngineDuo;
  if(name == "SuperGrafx"   ) information.model = Model::SuperGrafx;

  node = Node::System::create(name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;

  regionNode = node->append<Node::Setting::String>("Region", "NTSC-U → NTSC-J");
  regionNode->setAllowedValues({
    "NTSC-J → NTSC-U",
    "NTSC-U → NTSC-J",
    "NTSC-J",
    "NTSC-U"
  });

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
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC-J") {
      information.region = Region::NTSCJ;
    }
    if(region == "NTSC-U") {
      information.region = Region::NTSCU;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    if(have == cartridge.region()) setRegion(have);
  }

  if(PCD::Present()) pcd.power();
  cartridgeSlot.power();
  cpu.power();
  vdp.power();
  psg.power();
  scheduler.power(cpu);
}

}
