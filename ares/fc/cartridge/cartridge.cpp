#include <fc/fc.hpp>

namespace ares::Famicom {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "board/board.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(system.name());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};
  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();
  information.region = document["game/region"].string();

  board = Board::Interface::create(information.manifest);
  board->load();

  power();
  if(fds.present) {
    fds.load(node);
  }
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  if(fds.present) {
    fds.unload();
    fds.present = 0;
  }
  board->unload();
  board = {};
  node = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  board->save();
}

auto Cartridge::power() -> void {
  Thread::create(system.frequency(), {&Cartridge::main, this});
  board->power();
}

auto Cartridge::main() -> void {
  board->main();
}

auto Cartridge::readPRG(n32 address, n8 data) -> n8 {
  return board->readPRG(address, data);
}

auto Cartridge::writePRG(n32 address, n8 data) -> void {
  return board->writePRG(address, data);
}

auto Cartridge::readCHR(n32 address, n8 data) -> n8 {
  return board->readCHR(address, data);
}

auto Cartridge::writeCHR(n32 address, n8 data) -> void {
  return board->writeCHR(address, data);
}

auto Cartridge::scanline(n32 y) -> void {
  return board->scanline(y);
}

}
