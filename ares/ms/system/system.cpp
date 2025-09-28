#include <ms/ms.hpp>
#include <algorithm>

namespace ares::MasterSystem {

auto enumerate() -> std::vector<string> {
  return {
    "[Sega] Mark III (NTSC-J)",
    "[Sega] Master System (NTSC-J)",
    "[Sega] Master System (NTSC-U)",
    "[Sega] Master System (PAL)",
    "[Sega] Master System II (NTSC-U)",
    "[Sega] Master System II (PAL)",
    "[Sega] Game Gear (NTSC-J)",
    "[Sega] Game Gear (NTSC-U)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

Scheduler scheduler;
BIOS bios;
System system;
#include "bios.cpp"
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.pak) {
    return cartridge.title();
  }

  if(bios.rom) {
    return "(BIOS)";
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
  if(name.find("Mark III")) {
    information.name = "Master System";
    information.model = Model::MarkIII;
  }
  if(name.find("Master System")) {
    information.name = "Master System";
    information.model = Model::MasterSystemI;
  }
  if(name.find("Master System II")) {
    information.name = "Master System";
    information.model = Model::MasterSystemII;
  }
  if(name.find("Game Gear")) {
    information.name = "Game Gear";
    information.model = Model::GameGear;
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
    information.colorburst = Constants::Colorburst::NTSC;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
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

  scheduler.reset();
  controls.load(node);
  bios.load(node);
  cartridgeSlot.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  if(Device::MasterSystem()) {
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
  cartridgeSlot.unload();
  cpu.unload();
  vdp.unload();
  psg.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  if(Device::MasterSystem()) {
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
  }
  bios.unload();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if(cartridge.pak) {
    information.ms = cartridge.pak->attribute("ms").boolean();
  }

  bios.power();
  cartridge.power();
  cpu.power();
  vdp.power();
  psg.power();
  controllerPort1.power();
  controllerPort2.power();
  if(Device::MasterSystem()) {
    if(MasterSystem::Region::NTSCJ()) {
      if(MasterSystem::Model::MarkIII()) {
        expansionPort.power();
      }
      if(MasterSystem::Model::MasterSystemI()) {
        opll.power();
      }
      if(MasterSystem::Model::MasterSystemII()) {
        opll.power();
      }
    }
  }
  scheduler.power(cpu);
}

}
