#include <pce/pce.hpp>

namespace ares::PCEngine {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(interface->name());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();
  information.region = document["game/region"].string();
  information.board = document["game/board"].string();

  if(information.board == "Linear") board = new Board::Linear{*this};
  if(information.board == "Split") board = new Board::Split{*this};
  if(information.board == "Banked") board = new Board::Banked{*this};
  if(information.board == "RAM") board = new Board::RAM{*this};
  if(information.board == "System Card") board = new Board::SystemCard{*this};
  if(information.board == "Super System Card") board = new Board::SuperSystemCard{*this};
  if(information.board == "Arcade Card Duo") board = new Board::ArcadeCardDuo{*this};
  if(information.board == "Arcade Card Pro") board = new Board::ArcadeCardPro{*this};
  if(!board) board = new Board::Interface{*this};
  board->load(document);

  if(auto fp = platform->open(node, "backup.ram", File::Read)) {
    pcd.bram.load(fp);
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node || !board) return;
  board->unload();
  board = {};
  node = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);
  board->save(document);

  if(auto fp = platform->open(node, "backup.ram", File::Write)) {
    pcd.bram.save(fp);
  }
}

auto Cartridge::power() -> void {
  board->power();
}

auto Cartridge::read(uint8 bank, uint13 address, uint8 data) -> uint8 {
  return board->read(bank, address, data);
}

auto Cartridge::write(uint8 bank, uint13 address, uint8 data) -> void {
  return board->write(bank, address, data);
}

}
