#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Interface* interface = nullptr;

auto PlayStationInterface::game() -> string {
  if(disc.cd) {
    return disc.name();
  }

  return "(no disc inserted)";
}

auto PlayStationInterface::root() -> Node::Object {
  return system.node;
}

auto PlayStationInterface::load(Node::Object& root) -> void {
  interface = this;
  system.load(root);
}

auto PlayStationInterface::unload() -> void {
  system.unload();
}

auto PlayStationInterface::save() -> void {
  system.save();
}

auto PlayStationInterface::power() -> void {
  system.power(false);
}

auto PlayStationInterface::run() -> void {
  system.run();
}

auto PlayStationInterface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto PlayStationInterface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

}
