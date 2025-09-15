#include <md/md.hpp>

namespace ares::MegaDrive {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Mega Drive Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title    = pak->attribute("title");
  auto region = pak->attribute("region");
  information.regions  = nall::split_and_strip(region, ",");
  information.bootable = pak->attribute("bootable").boolean();

  if(pak->attribute("mega32x").boolean()) {
    board = std::make_unique<Board::Mega32X>(*this);
  } else if(pak->read("svp.rom")) {
    board = std::make_unique<Board::SVP>(*this);
  } else if(pak->attribute("label") == "Game Genie") {
    board = std::make_unique<Board::GameGenie>(*this);
  } else if(pak->attribute("jcart").boolean()) {
    board = std::make_unique<Board::JCart>(*this);
  } else if(pak->attribute("board") == "REALTEC") {
    board = std::make_unique<Board::Realtec>(*this);
  } else {
    board = std::make_unique<Board::Standard>(*this);
  }
  board->pak = pak;
  board->load();

  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  board->unload();
  board->pak.reset();
  board.reset();
  child.reset();
  pak.reset();
  node.reset();
  information = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  board->save();
}

auto Cartridge::main() -> void {
  board->main();
}

auto Cartridge::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto Cartridge::power(bool reset) -> void {
  if(!board) {
    if(Mega32X()) {
      board = std::make_unique<Board::Mega32X>(*this);
    } else {
      board = std::make_unique<Board::Interface>(*this);
    }

    board->load();
  }
  Thread::create(board->frequency(), std::bind_front(&Cartridge::main, this));
  board->power(reset);
}

auto Cartridge::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  return board->read(upper, lower, address, data);
}

auto Cartridge::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  return board->write(upper, lower, address, data);
}

auto Cartridge::readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  return board->readIO(upper, lower, address, data);
}

auto Cartridge::writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {
  return board->writeIO(upper, lower, address, data);
}

auto Cartridge::vblank(bool line) -> void {
  return board->vblank(line);
}

auto Cartridge::hblank(bool line) -> void {
  return board->hblank(line);
}

}
