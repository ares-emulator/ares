#include <gb/gb.hpp>

namespace ares::GameBoy {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "memory.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{parent->family(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  information.board = pak->attribute("board");

  board.reset();
  if(information.board == "HuC1"  ) board = new Board::HuC1{*this};
  if(information.board == "HuC3"  ) board = new Board::HuC3{*this};
  if(information.board == "MBC1"  ) board = new Board::MBC1{*this};
  if(information.board == "MBC1#M") board = new Board::MBC1M{*this};
  if(information.board == "MBC2"  ) board = new Board::MBC2{*this};
  if(information.board == "MBC3"  ) board = new Board::MBC3{*this};
  if(information.board == "MBC30" ) board = new Board::MBC3{*this};
  if(information.board == "MBC5"  ) board = new Board::MBC5{*this};
  if(information.board == "MBC6"  ) board = new Board::MBC6{*this};
  if(information.board == "MBC7"  ) board = new Board::MBC7{*this};
  if(information.board == "MMM01" ) board = new Board::MMM01{*this};
  if(information.board == "TAMA"  ) board = new Board::TAMA{*this};
  if(!board) board = new Board::Linear{*this};
  board->pak = pak;
  board->load();

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  board->unload();
  board->pak.reset();
  board.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  board->save();
}

auto Cartridge::power() -> void {
  Thread::create(4 * 1024 * 1024, {&Cartridge::main, this});

  bootromEnable = true;
  if(!board) board = new Board::None{*this};
  board->power();
}

auto Cartridge::main() -> void {
  board->main();
}

auto Cartridge::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

}
