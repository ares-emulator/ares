#include <pce/pce.hpp>

namespace ares::PCEngine {

Scheduler scheduler;
System system;
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) vdp.refresh();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "PC Engine"    ) information.model = Model::PCEngine;
  if(interface->name() == "PC Engine Duo") information.model = Model::PCEngineDuo;
  if(interface->name() == "SuperGrafx"   ) information.model = Model::SuperGrafx;

  node = Node::System::create(interface->name());
  root = node;

  regionNode = node->append<Node::String>("Region", "NTSC-U → NTSC-J");
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

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

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

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
