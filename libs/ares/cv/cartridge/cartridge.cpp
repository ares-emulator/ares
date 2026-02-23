#include <cv/cv.hpp>

namespace ares::ColecoVision {

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

  if(information.board == "xin1") board = std::make_unique<Board::xin1>(*this);
  if(information.board == "coleco") board = std::make_unique<Board::coleco>(*this);
  if(information.board == "megacart") board = std::make_unique<Board::megacart>(*this);
  
  
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
  if(board) return board->read(address);
  return 0;
}

auto Cartridge::write(n16 address, n8 data) -> void {
  if(board) return board->write(address, data);
}

}
