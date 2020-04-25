struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }
//auto board() const -> string { return information.board; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(uint8 bank, uint13 address, uint8 data) -> uint8;
  auto write(uint8 bank, uint13 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  unique_pointer<Board::Interface> board;

private:
  struct Information {
    string manifest;
    string name;
    string region;
    string board;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
