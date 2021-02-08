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

  auto read(u32 mode, n32 address) -> n32;
  auto write(u32 mode, n32 address, n32 word) -> void;

  auto serialize(serializer&) -> void;

private:
  struct Information {
    string manifest;
    string name;
  } information;

  struct Has {
    n1 sram;
    n1 eeprom;
    n1 flash;
  } has;
};

#include "slot.hpp"
extern Cartridge& cartridge;
