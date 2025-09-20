struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  //memory.cpp
  auto read(u32 cycle, n16 address, n8 data) -> n8;
  auto write(u32 cycle, n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
    string board;
  } information;

  //todo: this shouldn't be handled by the Cartridge class
  bool bootromEnable = true;
  bool transferPak = false; // Set when connected to an N64 via transfer pak

  std::unique_ptr<Board::Interface> board;
};

#include "slot.hpp"
extern Cartridge& cartridge;
