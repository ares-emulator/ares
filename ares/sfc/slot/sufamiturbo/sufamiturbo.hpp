struct SufamiTurboCartridge {
  Node::Port port;
  Node::Peripheral node;

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }

  //sufamiturbo.cpp
  auto load(Node::Peripheral, Node::Peripheral) -> void;
  auto unload() -> void;

  auto connect(Node::Peripheral) -> void;
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

extern SufamiTurboCartridge sufamiturboA;
extern SufamiTurboCartridge sufamiturboB;
