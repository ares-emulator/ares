#include "flash.hpp"

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;
  Flash flash[2];

  struct Debugger {
    Cartridge& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
    } memory;
  } debugger{*this};

  auto title() const -> string { return information.title; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;
  auto save() -> void;
  auto power() -> void;

  auto read(n1 bank, n21 address) -> n8;
  auto write(n1 bank, n21 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string title;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
