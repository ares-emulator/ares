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

  auto read(n16 address) -> n8;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string title;
  } information;

  Memory::Readable<n8> rom;
};

#include "slot.hpp"
extern Cartridge& cartridge;
