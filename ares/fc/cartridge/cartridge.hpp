struct Cartridge;
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Peripheral node;

  auto rate() const -> u32 { return Region::PAL() ? 16 : 12; }
  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto save() -> void;
  auto main() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
    string region;
  } information;

//privileged:
  unique_pointer<Board::Interface> board;

  auto readPRG(n32 address, n8 data) -> n8;
  auto writePRG(n32 address, n8 data) -> void;

  auto readCHR(n32 address, n8 data = 0x00) -> n8;
  auto writeCHR(n32 address, n8 data) -> void;

  //scanline() is for debugging purposes only:
  //boards must detect scanline edges on their own
  auto scanline(n32 y) -> void;
};

#include "slot.hpp"
extern Cartridge& cartridge;
