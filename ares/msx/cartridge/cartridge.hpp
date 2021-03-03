struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto title() const -> string { return information.title; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto read(n16 address) -> n8;
  auto write(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  unique_pointer<Board::Interface> board;

private:
  struct Information {
    string title;
    string region;
    string board;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
extern Cartridge& expansion;
