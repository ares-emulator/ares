#include <n64/n64.hpp>

namespace ares::Nintendo64 {

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
  information.cic    = pak->attribute("cic");

  const auto board_name = information.board;

  if((board_name == "NUS-01A-01") || (board_name == "NUS-01A-02") || (board_name == "NUS-01A-03")) {
    board = new Board::NUS_01A(*this);
  } else if((board_name == "NUS-07A-01")) {
    board = new Board::NUS_07A(*this);
  } else {
    board = new Board::Generic(*this);
  }

  board->pak = pak;
  board->load(node);

  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  board->unload(node);
  board->pak.reset();
  board.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  board->save();
}

auto Cartridge::power(bool reset) -> void {
  if(!board) {
    board = new Board::Interface(*this);
    board->load(node);
  }
  board->power(reset);
}

auto Cartridge::readHalf(u32 address) -> u16 {
  return board->readBus(address);
}

auto Cartridge::writeHalf(u32 address, u16 data) -> void {
  return board->writeBus(address, data);
}

}
