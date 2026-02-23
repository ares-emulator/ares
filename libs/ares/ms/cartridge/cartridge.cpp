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

  if(information.board == "Sega") board = std::make_unique<Board::Sega>(*this);
  if(information.board == "Codemasters") board = std::make_unique<Board::Codemasters>(*this);
  if(information.board == "Korea") board = std::make_unique<Board::Korea>(*this);
  if(information.board == "Zemina") board = std::make_unique<Board::Zemina>(*this, 0); // Zemina made games
  if(information.board == "Zemina_Nemesis") board = std::make_unique<Board::Zemina>(*this, 1); // Zemina's Nemesis port only
  if(information.board == "Janggun") board = std::make_unique<Board::Janggun>(*this);
  if(information.board == "Hicom") board = std::make_unique<Board::Hicom>(*this);
  if(information.board == "pak4") board = std::make_unique<Board::Pak4>(*this); // 4 PAK All Action (Australia)
  if(information.board == "Hap2000") board = std::make_unique<Board::Hap2000>(*this); // 2 Hap in 1 - David-2 ~ Moai-ui bomul (KR)
  if(information.board == "K188in1") board = std::make_unique<Board::K188in1>(*this); // Korean 188 in 1
  
  // WIP
  
  if(information.board == "Korea_NB") board = std::make_unique<Board::Korea>(*this); // TODO: Some of these games work but should not have bank switching.
  
  
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

auto Cartridge::read(n16 address, n8 data) -> n8 {
  if(board) return board->read(address, data);
  return data;
}

auto Cartridge::write(n16 address, n8 data) -> void {
  if(board) return board->write(address, data);
}

}
