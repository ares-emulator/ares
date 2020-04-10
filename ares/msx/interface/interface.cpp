#include <msx/msx.hpp>

namespace ares::MSX {

Interface* interface = nullptr;

auto AbstractInterface::game() -> string {
  if(cartridge.node && expansion.node) {
    return {cartridge.name(), " + ", expansion.name()};
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  if(expansion.node) {
    return expansion.name();
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
  system.run();
}

auto AbstractInterface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto AbstractInterface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

}
