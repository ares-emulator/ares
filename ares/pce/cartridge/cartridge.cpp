#include <pce/pce.hpp>

namespace ares::PCEngine {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Card"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");
  information.board  = pak->attribute("board");

  if(information.board == "Linear") board = new Board::Linear{*this};
  if(information.board == "Split") board = new Board::Split{*this};
  if(information.board == "Banked") board = new Board::Banked{*this};
  if(information.board == "RAM") board = new Board::RAM{*this};
  if(information.board == "System Card") board = new Board::SystemCard{*this};
  if(information.board == "Super System Card") board = new Board::SuperSystemCard{*this};
  if(information.board == "Arcade Card Duo") board = new Board::ArcadeCardDuo{*this};
  if(information.board == "Arcade Card Pro") board = new Board::ArcadeCardPro{*this};
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
  board->save();
}

auto Cartridge::power() -> void {
  board->power();
}

auto Cartridge::read(n8 bank, n13 address, n8 data) -> n8 {
  return board->read(bank, address, data);
}

auto Cartridge::write(n8 bank, n13 address, n8 data) -> void {
  return board->write(bank, address, data);
}

}
