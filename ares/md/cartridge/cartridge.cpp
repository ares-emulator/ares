#include <md/md.hpp>

namespace ares::MegaDrive {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(interface->name());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);

  information.name = document["game/label"].string();
  information.regions = document["game/region"].string().split(",").strip();
  information.bootable = (bool)document["game/bootable"];

  if(document["game/board/memory(type=ROM,content=Patch)"]) {
    board = new Board::LockOn(*this);
  } else if(document["game/board/slot(type=Mega Drive)"]) {
    board = new Board::GameGenie(*this);
  } else if(document["game/board/memory(type=ROM,content=Program)/size"].natural() > 0x200000) {
    board = new Board::Banked(*this);
  } else {
    board = new Board::Linear(*this);
  }
  board->load(document);
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  board = {};
  node = {};
  information = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);
  board->save(document);
}

auto Cartridge::power() -> void {
  board->power();
}

auto Cartridge::read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  return board->read(upper, lower, address, data);
}

auto Cartridge::write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
  return board->write(upper, lower, address, data);
}

auto Cartridge::readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  return board->readIO(upper, lower, address, data);
}

auto Cartridge::writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  return board->writeIO(upper, lower, address, data);
}

}
