#include <ng/ng.hpp>

namespace ares::NeoGeo {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Neo Geo");
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);
}

auto Cartridge::power() -> void {
}

}
