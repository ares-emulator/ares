struct SufamiTurboCartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }

  //sufamiturbo.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto save() -> void;

  //memory.cpp
  auto readROM(n24 address, n8 data) -> n8;
  auto writeROM(n24 address, n8 data) -> void;

  auto readRAM(n24 address, n8 data) -> n8;
  auto writeRAM(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  ReadableMemory rom;
  WritableMemory ram;

  struct {
    string title;
  } information;
};

#include "slot.hpp"
extern SufamiTurboCartridge& sufamiturboA;
extern SufamiTurboCartridge& sufamiturboB;
