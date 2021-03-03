#include <fc/fc.hpp>

namespace ares::Famicom {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "board/board.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");

  board = Board::Interface::create(pak->attribute("board"));
  board->pak = pak;
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
  board->pak.reset();
  board.reset();
  pak.reset();
  node.reset();
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
