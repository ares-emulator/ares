#include <ms/ms.hpp>

namespace ares::MasterSystem {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");
  information.board  = pak->attribute("board");

  if(information.board == "Sega") board = new Board::Sega{*this};
  if(information.board == "Codemasters") board = new Board::Codemasters{*this};
  if(information.board == "Korea") board = new Board::Korea{*this};
  if(information.board == "MSX") board = new Board::MSX{*this, 0};
  if(information.board == "MSX#A") board = new Board::MSX{*this, 1};
  if(information.board == "Janggun") board = new Board::Janggun{*this};
  if(!board) board = new Board::Interface{*this};
  board->pak = pak;
  board->load();
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node || !board) return;
  board->unload();
  board->pak.reset();
  board.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  if(board) board->save();
}

auto Cartridge::power() -> void {
  if(board) board->power();
}

auto Cartridge::read(n16 address, n8 data) -> n8 {
  if(board) return board->read(address, data);
  return data;
}

auto Cartridge::write(n16 address, n8 data) -> void {
  if(board) return board->write(address, data);
}

}
