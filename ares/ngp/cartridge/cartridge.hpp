#include "flash.hpp"

struct Cartridge {
  Node::Port port;
  Node::Peripheral node;

  Flash flash[2];

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }

  //cartridge.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
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

extern Cartridge cartridge;
