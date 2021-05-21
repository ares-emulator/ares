struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Readable<n16> prom;
  Memory::Readable<n8 > mrom;
  Memory::Readable<n8 > crom;
  Memory::Readable<n8 > srom;
  Memory::Readable<n8 > vrom;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory prom;
      Node::Debugger::Memory mrom;
      Node::Debugger::Memory crom;
      Node::Debugger::Memory srom;
      Node::Debugger::Memory vrom;
    } memory;
  } debugger;

  auto title() const -> string { return information.title; }

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
    string title;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
