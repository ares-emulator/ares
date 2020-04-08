struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //cartridge.cpp
  auto connect(Node::Port, Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(uint8 bank, uint13 address) -> uint8;
  auto write(uint8 bank, uint13 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  unique_pointer<Board::Interface> board;

private:
  struct Information {
    string manifest;
    string name;
    string board;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
