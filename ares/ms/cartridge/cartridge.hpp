struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  //mapper.cpp
  auto read(n16 address) -> maybe<n8>;
  auto write(n16 address, n8 data) -> bool;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Information {
    string title;
    string region;
  } information;

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  struct Mapper {
    //$fffc
    n2 shift;
    n1 ramPage2;
    n1 ramEnablePage2;
    n1 ramEnablePage3;
    n1 romWriteEnable;

    //$fffd
    n8 romPage0 = 0;

    //$fffe
    n8 romPage1 = 1;

    //$ffff
    n8 romPage2 = 2;
  } mapper;
};

#include "slot.hpp"
extern Cartridge& cartridge;
