#include <saturn/saturn.hpp>
#include <algorithm>

namespace ares::Saturn {

auto enumerate() -> std::vector<string> {
  return {
    "[Sega] Saturn (NTSC-J)",
    "[Sega] Saturn (NTSC-U)",
    "[Sega] Saturn (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

System system;
#include "serialization.cpp"

auto System::game() -> string {
  return "(no disc loaded)";
}

auto System::run() -> void {
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Saturn")) {
    information.name = "Saturn";
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
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

  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();
}

}
