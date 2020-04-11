struct Cartridge {
  Node::Peripheral node;

  #include "memory.hpp"

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //cartridge.cpp
  Cartridge();
  ~Cartridge();

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(uint mode, uint32 address) -> uint32;
  auto write(uint mode, uint32 address, uint32 word) -> void;

  auto serialize(serializer&) -> void;

private:
  struct Information {
    string manifest;
    string name;
  } information;

  struct Has {
    uint1 sram;
    uint1 eeprom;
    uint1 flash;
  } has;
};

#include "slot.hpp"
extern Cartridge& cartridge;
