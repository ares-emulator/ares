struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void;

  auto readM(n32 address) -> n8;
  auto readC(n32 address) -> n8;
  auto readS(n32 address) -> n8;
  auto readVA(n32 address) -> n8;
  auto readVB(n32 address) -> n8;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  std::unique_ptr<Board::Interface> board;

private:
  struct Information {
    string title;
    string board;
  } information;

  n8 bank;
};

#include "slot.hpp"
extern Cartridge& cartridge;
