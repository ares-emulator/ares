struct Cartridge {
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
  auto power() -> void;

  auto read(uint16 address) -> maybe<uint8>;
  auto write(uint16 address, uint8 data) -> bool;

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
