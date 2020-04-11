struct SufamiTurboCartridge {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //sufamiturbo.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto save() -> void;

  //memory.cpp
  auto readROM(uint24 address, uint8 data) -> uint8;
  auto writeROM(uint24 address, uint8 data) -> void;

  auto readRAM(uint24 address, uint8 data) -> uint8;
  auto writeRAM(uint24 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  ReadableMemory rom;
  WritableMemory ram;

  struct {
    string manifest;
    string name;
  } information;
};

#include "slot.hpp"
extern SufamiTurboCartridge& sufamiturboA;
extern SufamiTurboCartridge& sufamiturboB;
