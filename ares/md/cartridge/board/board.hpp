namespace Board {

struct Interface {
  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}
  virtual auto read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 { return 0; }
  virtual auto write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {}
  virtual auto readIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 { return 0; }
  virtual auto writeIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<uint16>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<uint16>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable< uint8>& memory, Markup::Node node) -> bool;

  auto save(Memory::Writable<uint16>& memory, Markup::Node node) -> bool;
  auto save(Memory::Writable< uint8>& memory, Markup::Node node) -> bool;

  maybe<Cartridge&> cartridge;
};

}
