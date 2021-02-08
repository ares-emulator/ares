struct Cartridge {
  Node::Peripheral node;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(n16 address) -> maybe<n8>;
  auto write(n16 address, n8 data) -> bool;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Information {
    string manifest;
    string name;
    string region;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
