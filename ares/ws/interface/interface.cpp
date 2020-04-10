#include <ws/ws.hpp>

namespace ares::WonderSwan {

Interface* interface = nullptr;

auto AbstractInterface::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto AbstractInterface::root() -> Node::Object {
  return system.node;
}

auto AbstractInterface::load(Node::Object& root) -> void {
  interface = this;
  system.load(root);
}

auto AbstractInterface::unload() -> void {
  system.unload();
}

auto AbstractInterface::save() -> void {
  system.save();
}

auto AbstractInterface::power() -> void {
  system.power();
}

auto AbstractInterface::run() -> void {
  return system.run();
}

auto AbstractInterface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto AbstractInterface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

}
