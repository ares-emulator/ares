#include <a52/a52.hpp>

namespace ares::Atari5200 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Atari 5200 Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  information.board = pak->attribute("board");

  if(information.board == "Linear32K") board = std::make_unique<Board::Linear32K>(*this);
  if(!board) return;

  board->pak = pak;
  if(!board->load()) {
    board.reset();
    return;
  }
}

auto Cartridge::disconnect() -> void {
  if(board) board->unload();
  board.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
}

auto Cartridge::power() -> void {
  if(board) board->power();
}

auto Cartridge::read(n16 address, n8 data) -> n8 {
  if(!board) return data;
  return board->read(address, data);
}

auto Cartridge::peek(n16 address, n8 data) const -> n8 {
  if(!board) return data;
  return board->peek(address, data);
}

auto Cartridge::write(n16 address, n8 data) -> bool {
  if(!board) return false;
  return board->write(address, data);
}

}
