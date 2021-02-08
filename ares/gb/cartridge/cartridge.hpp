struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
//auto board() const -> string { return information.board; }

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
    string manifest;
    string name;
    string board;
  } information;

  //todo: this shouldn't be handled by the Cartridge class
  bool bootromEnable = true;

  unique_pointer<Board::Interface> board;
};

#include "slot.hpp"
extern Cartridge& cartridge;
