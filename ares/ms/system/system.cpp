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
  if(name == "Mark III"        ) information.model = Model::MarkIII;
  if(name == "Master System I" ) information.model = Model::MasterSystemI;
  if(name == "Master System II") information.model = Model::MasterSystemII;
  if(name == "Game Gear"       ) information.model = Model::GameGear;

  node = Node::System::create(name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;

  regionNode = node->append<Node::Setting::String>("Region", "NTSC-J → NTSC-U → PAL");
  regionNode->setAllowedValues({
    "NTSC-J → NTSC-U → PAL",
    "NTSC-U → NTSC-J → PAL",
    "PAL → NTSC-J → NTSC-U",
    "PAL → NTSC-U → NTSC-J",
    "NTSC-J",
    "NTSC-U",
    "PAL"
  });

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  if(MasterSystem::Model::MasterSystem()) {
    controllerPort1.load(node);
    controllerPort2.load(node);
  }
  if(MasterSystem::Region::NTSCJ()) {
    if(MasterSystem::Model::MarkIII()) {
      expansionPort.load(node);
    }
    if(MasterSystem::Model::MasterSystemI()) {
      opll.load(node);
    }
    if(MasterSystem::Model::MasterSystemII()) {
      opll.load(node);
    }
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
  cartridgeSlot.unload();
  if(MasterSystem::Model::MasterSystem()) {
    controllerPort1.unload();
    controllerPort2.unload();
  }
  if(MasterSystem::Region::NTSCJ()) {
    if(MasterSystem::Model::MarkIII()) {
      expansionPort.unload();
    }
    if(MasterSystem::Model::MasterSystemI()) {
      opll.unload();
    }
    if(MasterSystem::Model::MasterSystemII()) {
      opll.unload();
    }
  }
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC-J") {
      information.region = Region::NTSCJ;
      information.colorburst = Constants::Colorburst::NTSC;
    }
    if(region == "NTSC-U") {
      information.region = Region::NTSCU;
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
  if(MasterSystem::Model::MasterSystem()) {
    controllerPort1.power();
    controllerPort2.power();
  }
  if(MasterSystem::Region::NTSCJ()) {
    if(MasterSystem::Model::MasterSystemI()) {
      opll.power();
    }
    if(MasterSystem::Model::MasterSystemII()) {
      opll.power();
    }
  }
  scheduler.power(cpu);

  //todo: this is a hack because load() attaches the OPLL before the region is configured.
  //this incorrect fix can crash the emulator if the region is changed without restarting!
  //the correct fix will require games to set the system region before calling load()
  if(opll.node && !MasterSystem::Region::NTSCJ()) {
    opll.unload();
  }
}

}
