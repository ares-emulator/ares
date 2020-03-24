#include <ms/ms.hpp>

namespace ares::MasterSystem {

Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) {
    vdp.refresh();
    controls.poll();
  }
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "Master System") information.model = Model::MasterSystem;
  if(interface->name() == "Game Gear"    ) information.model = Model::GameGear;

  node = Node::append<Node::System>(nullptr, from, interface->name());
  root = node;

  regionNode = Node::append<Node::String>(node, from, "Region", "NTSC → PAL");
  regionNode->setAllowedValues({
    "NTSC → PAL",
    "PAL → NTSC",
    "NTSC",
    "PAL"
  });

  scheduler.reset();
  controls.load(node, from);
  cpu.load(node, from);
  vdp.load(node, from);
  psg.load(node, from);
  opll.load(node, from);
  cartridge.load(node, from);
  if(!MasterSystem::Model::GameGear()) {
    controllerPort1.load(node, from);
    controllerPort2.load(node, from);
  }
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
  opll.unload();
  cartridge.unload();
  if(!MasterSystem::Model::GameGear()) {
    controllerPort1.unload();
    controllerPort2.unload();
  }
  node = {};
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC") {
      information.region = Region::NTSC;
      information.colorburst = Constants::Colorburst::NTSC;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
      information.colorburst = Constants::Colorburst::PAL * 4.0 / 5.0;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    if(have == cartridge.region()) setRegion(have);
  }

  cartridge.power();
  cpu.power();
  vdp.power();
  psg.power();
  opll.power();
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
