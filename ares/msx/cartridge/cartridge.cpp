#include <msx/msx.hpp>

namespace ares::MSX {

Cartridge& cartridge = cartridgeSlot.cartridge;
Cartridge& expansion = expansionSlot.cartridge;
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

  if(information.board == "ASC16") board = std::make_unique<Board::ASC16>(*this, false);
  if(information.board == "ASC16R") board = std::make_unique<Board::ASC16>(*this, true);
  if(information.board == "ASC8") board = std::make_unique<Board::ASC8>(*this);
  if(information.board == "CrossBlaim") board = std::make_unique<Board::CrossBlaim>(*this);
  if(information.board == "Konami") board = std::make_unique<Board::Konami>(*this);
  if(information.board == "KonamiSCC") board = std::make_unique<Board::KonamiSCC>(*this);
  if(information.board == "Linear") board = std::make_unique<Board::Linear>(*this, 0x4000);
  if(information.board == "LinearPage2") board = std::make_unique<Board::Linear>(*this, 0x8000);
  if(information.board == "Mirrored") board = std::make_unique<Board::Mirrored>(*this);
  if(information.board == "SuperLodeRunner") board = std::make_unique<Board::SuperLodeRunner>(*this);
  if(information.board == "SuperPierrot") board = std::make_unique<Board::SuperPierrot>(*this);
  if(!board) board = std::make_unique<Board::Konami>(*this);
  board->pak = pak;
  board->load();

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  if(board) board->unload();
  board.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  if(board) board->save();
}

auto Cartridge::main() -> void {
  if(board) return board->main();
  step(system.colorburst());
}

auto Cartridge::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto Cartridge::power() -> void {
  Thread::create(system.colorburst(), std::bind_front(&Cartridge::main, this));
  if(board) board->power();
}

auto Cartridge::read(n16 address) -> n8 {
  if(board) return board->read(address, 0xff);
  return 0xff;
}

auto Cartridge::write(n16 address, n8 data) -> void {
  if(board) return board->write(address, data);
}

}
