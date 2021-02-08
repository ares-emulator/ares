struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto regions() const -> vector<string> { return information.regions; }
  auto bootable() const -> boolean { return information.bootable; }  //CART_IN line

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16;
  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void;

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
    vector<string> regions;
    boolean bootable;
  } information;

  unique_pointer<Board::Interface> board;
};

#include "slot.hpp"
extern Cartridge& cartridge;
