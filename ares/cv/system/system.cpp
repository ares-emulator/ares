#include <cv/cv.hpp>
#include <algorithm>

namespace ares::ColecoVision {

auto enumerate() -> std::vector<string> {
  return {
    "[Coleco] ColecoVision (NTSC)",
    "[Coleco] ColecoVision (PAL)",
    "[Coleco] ColecoAdam (NTSC)",
    "[Coleco] ColecoAdam (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
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

  if(auto fp = pak->read("bios.rom")) {
    fp->read(bios, 0x2000);
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
