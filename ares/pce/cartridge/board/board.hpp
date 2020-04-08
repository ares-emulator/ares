namespace Board {

struct Interface {
  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}
  virtual auto read(uint8 bank, uint13 address) -> uint8 { return 0x00; }
  virtual auto write(uint8 bank, uint13 address, uint8 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;

  auto save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;

  Cartridge& cartridge;
};

}
