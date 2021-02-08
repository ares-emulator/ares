namespace Board {

struct Interface {
  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}
  virtual auto unload() -> void {}
  virtual auto main() -> void;
  virtual auto step(u32 clocks) -> void;
  virtual auto read(n16 address, n8 data) -> n8 { return data; }
  virtual auto write(n16 address, n8 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n8>&, Markup::Node) -> bool;
  auto load(Memory::Writable<n8>&, Markup::Node) -> bool;
  auto save(Memory::Writable<n8>&, Markup::Node) -> bool;

  Cartridge& cartridge;
};

}
