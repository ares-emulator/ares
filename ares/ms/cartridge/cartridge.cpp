#include <ms/ms.hpp>

namespace ares::MasterSystem {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "mapper.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(system.name());
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  if(auto fp = pak->read("manifest.bml")) {
    information.manifest = fp->reads();
  }
  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = pak->read("program.rom")) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(!(bool)memory["volatile"]) {
      if(auto fp = pak->read("save.ram")) {
        ram.load(fp);
      }
    }
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  ram.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!(bool)memory["volatile"]) {
      if(auto fp = pak->write("save.ram")) {
        ram.save(fp);
      }
    }
  }
}

auto Cartridge::power() -> void {
  mapper = {};
}

}
