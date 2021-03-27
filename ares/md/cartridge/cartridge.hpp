struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }
  auto regions() const -> vector<string> { return information.regions; }
  auto bootable() const -> boolean { return information.bootable; }  //CART_IN line

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16;
  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void;

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  auto vblank(bool line) -> void;
  auto hblank(bool line) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
    vector<string> regions;
    boolean bootable;
  } information;

  unique_pointer<Board::Interface> board;
};

#include "slot.hpp"
extern Cartridge& cartridge;
