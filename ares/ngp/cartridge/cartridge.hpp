#include "flash.hpp"

struct Cartridge {
  Node::Peripheral node;
  Flash flash[2];

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;
  auto save() -> void;
  auto power() -> void;

  auto read(uint1 bank, uint21 address) -> uint8;
  auto write(uint1 bank, uint21 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string manifest;
    string name;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
