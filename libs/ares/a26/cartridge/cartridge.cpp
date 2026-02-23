#include <a26/a26.hpp>

namespace ares::Atari2600 {

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

  if(information.board == "Linear") board = std::make_unique<Board::Linear>(*this);
  if(information.board == "Atari8k") board = std::make_unique<Board::Atari8k>(*this);
  if(information.board == "Atari16k") board = std::make_unique<Board::Atari16k>(*this);
  if(information.board == "Atari32k") board = std::make_unique<Board::Atari32k>(*this);
  if(information.board == "Commavid") board = std::make_unique<Board::Commavid>(*this);
  if(information.board == "ParkerBros8k") board = std::make_unique<Board::ParkerBros>(*this);
  if(information.board == "Tigervision") board = std::make_unique<Board::Tigervision>(*this);

  if(!board) board = std::make_unique<Board::Interface>(*this);
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

auto Cartridge::read(n16 address) -> n8 {
  if(!node) return 0xff;
  if(board) return board->read(address);

  return 0xff;
}

auto Cartridge::write(n16 address, n8 data) -> bool {
  if(!node) return false;
  if(board) return board->write(address, data);
  return false;
}

}
