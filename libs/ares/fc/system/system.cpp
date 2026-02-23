#include <fc/fc.hpp>
#include <algorithm>

namespace ares::Famicom {

auto enumerate() -> std::vector<string> {
  return {
    "[Nintendo] Famicom (NTSC-J)",
    "[Nintendo] Famicom (NTSC-U)",
    "[Nintendo] Famicom (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(fds.node) {
    return fds.title();
  }

  if(cartridge.node) {
    return cartridge.title();
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
  if(name.find("NTSC-J")) {
    information.name = "Famicom";
    information.region = Region::NTSCJ;
    information.frequency = Constants::Colorburst::NTSC * 6.0;
  }
  if(name.find("NTSC-U")) {
    information.name = "Famicom";
    information.region = Region::NTSCU;
    information.frequency = Constants::Colorburst::NTSC * 6.0;
  }
  if(name.find("PAL")) {
    information.name = "Famicom";
    information.region = Region::PAL;
    information.frequency = Constants::Colorburst::PAL * 6.0;
  }

  node = std::make_shared<Core::System>(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  node->setSerialize([this](bool s){ return this->serialize(s); });
  node->setUnserialize([this](serializer& s){ return this->unserialize(s); });
  root = node;

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
  if(fds.present) fds.save();
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

  random.entropy(Random::Entropy::Low);
  // The apu should run before the cpu
  apu.power(reset);
  cartridge.power();
  cpu.power(reset);
  ppu.power(reset);
  scheduler.power(cpu);
}

}
