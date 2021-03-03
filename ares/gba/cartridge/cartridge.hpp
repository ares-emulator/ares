struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  #include "memory.hpp"

  auto title() const -> string { return information.title; }

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
    string title;
  } information;

  struct Has {
    n1 sram;
    n1 eeprom;
    n1 flash;
  } has;
};

#include "slot.hpp"
extern Cartridge& cartridge;
