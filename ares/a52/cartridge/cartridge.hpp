struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }

  auto allocate(Node::Port parent) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(n16 address, n8 data) -> n8;
  auto peek(n16 address, n8 data) const -> n8;
  auto write(n16 address, n8 data) -> bool;

  std::unique_ptr<Board::Interface> board;

private:
  struct Information {
    string title;
    string board;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
