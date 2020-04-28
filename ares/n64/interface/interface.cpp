#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Interface* interface = nullptr;

auto Nintendo64Interface::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto Nintendo64Interface::root() -> Node::Object {
  return system.node;
}

auto Nintendo64Interface::load(Node::Object& root) -> void {
  interface = this;
  system.load(root);
}

auto Nintendo64Interface::unload() -> void {
  system.unload();
}

auto Nintendo64Interface::save() -> void {
  system.save();
}

auto Nintendo64Interface::power() -> void {
  system.power(false);
}

auto Nintendo64Interface::run() -> void {
  system.run();
}

auto Nintendo64Interface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto Nintendo64Interface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

}
