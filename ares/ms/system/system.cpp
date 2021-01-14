#include <ms/ms.hpp>

namespace ares::MasterSystem {

auto load(Node::System& node, string name) -> bool {
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  controls.poll();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name == "Master System") information.model = Model::MasterSystem;
  if(name == "Game Gear"    ) information.model = Model::GameGear;

  node = Node::System::create(name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;

  regionNode = node->append<Node::Setting::String>("Region", "NTSC → PAL");
  regionNode->setAllowedValues({
    "NTSC → PAL",
    "PAL → NTSC",
    "NTSC",
    "PAL"
  });

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  opll.load(node);
  cartridgeSlot.load(node);
  if(!MasterSystem::Model::GameGear()) {
    controllerPort1.load(node);
    controllerPort2.load(node);
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
  opll.unload();
  cartridgeSlot.unload();
  if(!MasterSystem::Model::GameGear()) {
    controllerPort1.unload();
    controllerPort2.unload();
  }
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

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
}

}
