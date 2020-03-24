struct Cartridge {
  Node::Port port;
  Node::Peripheral node;

  Memory::Readable<uint16> rom;
  Memory::Readable<uint16> patch;
  Memory::Writable<uint16> wram;  //16-bit save RAM
  Memory::Writable< uint8> bram;  // 8-bit save RAM

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }
  inline auto regions() const -> vector<string> { return information.regions; }
  inline auto bootable() const -> boolean { return information.bootable; }  //CART_IN line

  //cartridge.cpp
  Cartridge() = default;
  Cartridge(uint depth) : depth(depth) {}

  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto loadROM(Memory::Readable<uint16>& rom, Markup::Node memory) -> bool;
  auto loadRAM(Memory::Writable<uint16>& ram, Markup::Node memory) -> bool;
  auto loadRAM(Memory::Writable<uint8>& ram, Markup::Node memory) -> bool;

  auto saveRAM(Memory::Writable<uint16>& ram, Markup::Node memory) -> bool;
  auto saveRAM(Memory::Writable<uint8>& ram, Markup::Node memory) -> bool;

  auto readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16;
  auto writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void;

  auto readLinear(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto writeLinear(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  auto readBanked(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto writeBanked(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  auto readLockOn(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto writeLockOn(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  auto readGameGenie(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto writeGameGenie(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
    vector<string> regions;
    boolean bootable;
  } information;

  uint1 ramUpper;
  uint1 ramLower;
  uint1 ramEnable;
  uint1 ramWritable;
  uint6 romBank[8];

  struct GameGenie {
    boolean enable;
    struct Code {
      boolean enable;
      uint24 address;
      uint16 data;
    } codes[5];
  } gameGenie;

  function<uint16 (uint1 upper, uint1 lower, uint22 address, uint16 data)> read;
  function<void (uint1 upper, uint1 lower, uint22 address, uint16 data)> write;

  unique_pointer<Cartridge> slot;
  const uint depth = 0;
};

extern Cartridge cartridge;
