#include <cv/cv.hpp>

namespace ares::ColecoVision {

auto enumerate() -> vector<string> {
  return {
    "[Coleco] ColecoVision (NTSC)",
    "[Coleco] ColecoVision (PAL)",
    "[Coleco] ColecoAdam (NTSC)",
    "[Coleco] ColecoAdam (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

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
  controls.poll();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("ColecoVision")) {
    information.name = "ColecoVision";
    information.model = Model::ColecoVision;
  }
  if(name.find("ColecoAdam")) {
    information.name = "ColecoAdam";
    information.model = Model::ColecoAdam;
  }
  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.colorburst = Constants::Colorburst::NTSC;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.colorburst = Constants::Colorburst::PAL * 4.0 / 5.0;
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

  if(auto fp = pak->read("bios.rom")) {
    fp->read({bios, 0x2000});
  }

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
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
  cpu.unload();
  vdp.unload();
  psg.unload();
  cartridgeSlot.unload();
  controllerPort1.port = {};
  controllerPort2.port = {};
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  cartridge.power();
  cpu.power();
  vdp.power();
  psg.power();
  scheduler.power(cpu);
}

}
