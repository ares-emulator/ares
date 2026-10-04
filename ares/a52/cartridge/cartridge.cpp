#include <a52/a52.hpp>

namespace ares::Atari5200 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Atari 5200 Cartridge");
}

auto Cartridge::connect() -> void {
  board.reset();
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  information.board = pak->attribute("board");

  using Wiring = Board::Linear::Wiring;
  if(information.board == "Linear4K"      ) board = std::make_unique<Board::Linear>(*this,  4_KiB, Wiring::Upper);
  if(information.board == "Linear8K"      ) board = std::make_unique<Board::Linear>(*this,  8_KiB, Wiring::Upper);
  if(information.board == "OneChip16K"    ) board = std::make_unique<Board::Linear>(*this, 16_KiB, Wiring::Full);
  if(information.board == "TwoChip16K"    ) board = std::make_unique<Board::Linear>(*this, 16_KiB, Wiring::Split);
  if(information.board == "Overlapping16K") board = std::make_unique<Board::Linear>(*this, 16_KiB, Wiring::Overlapping);
  if(information.board == "Linear32K"     ) board = std::make_unique<Board::Linear>(*this, 32_KiB, Wiring::Full);
  if(information.board == "DualWindow40K" ) board = std::make_unique<Board::DualWindow40K>(*this);
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
