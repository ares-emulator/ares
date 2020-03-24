#include "chip/chip.hpp"
#include "board/board.hpp"

struct Cartridge : Thread {
  Node::Port port;
  Node::Peripheral node;

  inline auto rate() const -> uint { return Region::PAL() ? 16 : 12; }
  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }
  inline auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
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
  Board* board = nullptr;

  auto readPRG(uint addr) -> uint8;
  auto writePRG(uint addr, uint8 data) -> void;

  auto readCHR(uint addr) -> uint8;
  auto writeCHR(uint addr, uint8 data) -> void;

  //scanline() is for debugging purposes only:
  //boards must detect scanline edges on their own
  auto scanline(uint y) -> void;
};

extern Cartridge cartridge;
