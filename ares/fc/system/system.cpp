#include <fc/fc.hpp>

namespace ares::Famicom {

auto load(Node::System& node, string name) -> bool {
  return system.load(node, name);
}

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(fds.node) {
    return fds.name();
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  auto reset = controls.reset->value();
  platform->input(controls.reset);
  if(!reset && controls.reset->value()) power(true);
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};

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
  apu.load(node);
  ppu.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  expansionPort.load(node);
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
  apu.unload();
  ppu.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  expansionPort.unload();
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC-J") {
      information.region = Region::NTSCJ;
      information.frequency = Constants::Colorburst::NTSC * 6.0;
    }
    if(region == "NTSC-U") {
      information.region = Region::NTSCU;
      information.frequency = Constants::Colorburst::NTSC * 6.0;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
      information.frequency = Constants::Colorburst::PAL * 6.0;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    if(have == cartridge.region()) setRegion(have);
  }

  //zero-initialization (breaks Super Mario Bros.)
  #if 0
  if(!reset) {
    serializer s;
    s.setReading();
    serialize(s, true);
  }
  #endif

  random.entropy(Random::Entropy::Low);
  cartridge.power();
  cpu.power(reset);
  apu.power(reset);
  ppu.power(reset);
  scheduler.power(cpu);
}

}
