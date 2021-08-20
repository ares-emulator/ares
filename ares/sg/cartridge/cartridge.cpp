#include <sg/sg.hpp>

namespace ares::SG1000 {

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


  if(information.board == "Linear") board = new Board::Linear{*this};
  if(information.board == "Taiwan-A") board = new Board::TaiwanA{*this};
  if(information.board == "Taiwan-B") board = new Board::TaiwanB{*this};

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

auto Cartridge::read(n16 address) -> maybe<n8> {
  if(!node) return nothing;
  if (board) return board->read(address);

  return nothing;
}

auto Cartridge::write(n16 address, n8 data) -> bool {
  if(!node) return false;
  if (board) return board->write(address, data);
  return false;
}

}
