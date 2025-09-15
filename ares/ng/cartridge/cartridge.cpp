#include <ng/ng.hpp>

namespace ares::NeoGeo {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Neo Geo Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  information.board = pak->attribute("board");

  if(information.board == "rom_mslugx") board = std::make_unique<Board::MSlugX>(*this);
  if(information.board == "cmc50_jockeygp") board = std::make_unique<Board::JockeyGP>(*this);
  if(!board) board = std::make_unique<Board::Rom>(*this);
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

auto Cartridge::readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(board) return board->readP(upper, lower, address, data);
  return data;
}

auto Cartridge::writeP(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(board) return board->writeP(upper, lower, address, data);
}

auto Cartridge::readM(n32 address) -> n8 {
 if(board) return board->readM(address);
 return 0xff;
}

auto Cartridge::readC(n32 address) -> n8 {
  if(board) return board->readC(address);
  return 0xff;
}

auto Cartridge::readS(n32 address) -> n8 {
  if(board) return board->readS(address);
  return 0xff;
}

auto Cartridge::readVA(n32 address) -> n8 {
  if(board) return board->readVA(address);
  return 0xff;
}

auto Cartridge::readVB(n32 address) -> n8 {
  if(board) return board->readVB(address);
  return 0xff;
}

}
