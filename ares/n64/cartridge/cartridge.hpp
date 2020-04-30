struct Cartridge {
  Node::Peripheral node;
  Memory::Readable<uint32> rom;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

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
