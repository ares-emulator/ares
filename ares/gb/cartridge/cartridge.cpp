#include <gb/gb.hpp>

namespace ares::GameBoy {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "memory.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(parent->family());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();
  information.board = document["game/board"].string();

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
  board->load(document);

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node || !board) return;
  board->unload();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);
  board->save(document);
}

auto Cartridge::power() -> void {
  Thread::create(4 * 1024 * 1024, {&Cartridge::main, this});

  bootromEnable = true;
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
