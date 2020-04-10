#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

Interface* interface = nullptr;

auto GameBoyAdvanceInterface::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto GameBoyAdvanceInterface::root() -> Node::Object {
  return system.node;
}

auto GameBoyAdvanceInterface::load(Node::Object& root) -> void {
  interface = this;
  system.load(root);
}

auto GameBoyAdvanceInterface::unload() -> void {
  system.unload();
}

auto GameBoyAdvanceInterface::save() -> void {
  system.save();
}

auto GameBoyAdvanceInterface::power() -> void {
  system.power();
}

auto GameBoyAdvanceInterface::run() -> void {
  system.run();
}

auto GameBoyAdvanceInterface::serialize(bool synchronize) -> serializer {
  return system.serialize(synchronize);
}

auto GameBoyAdvanceInterface::unserialize(serializer& s) -> bool {
  return system.unserialize(s);
}

}
