#include <saturn/saturn.hpp>

namespace ares::Saturn {

auto enumerate() -> vector<string> {
  return {
    "[Sega] Saturn (NTSC-J)",
    "[Sega] Saturn (NTSC-U)",
    "[Sega] Saturn (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
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
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
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
