namespace Board {

struct Interface {
  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}
  virtual auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 { return 0; }
  virtual auto write(n1 upper, n1 lower, n22 address, n16 data) -> void {}
  virtual auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 { return 0; }
  virtual auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n16>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<n16>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<n8 >& memory, Markup::Node node) -> bool;

  auto save(Memory::Writable<n16>& memory, Markup::Node node) -> bool;
  auto save(Memory::Writable<n8 >& memory, Markup::Node node) -> bool;

  maybe<Cartridge&> cartridge;
};

}
