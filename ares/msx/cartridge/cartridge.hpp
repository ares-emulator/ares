struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  auto read(uint16 address) -> uint8;
  auto write(uint16 address, uint8 data) -> void;

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
extern Cartridge& expansion;
