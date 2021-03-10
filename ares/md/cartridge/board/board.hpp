namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto frequency() -> u32 { return 1; }
  virtual auto load() -> void {}
  virtual auto unload() -> void {}
  virtual auto save() -> void {}
  virtual auto main() -> void;
  virtual auto step(u32 clocks) -> void;
  virtual auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 { return 0; }
  virtual auto write(n1 upper, n1 lower, n22 address, n16 data) -> void {}
  virtual auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 { return 0; }
  virtual auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n16>& memory, string name) -> bool;
  auto load(Memory::Writable<n16>& memory, string name) -> bool;
  auto load(Memory::Writable<n8 >& memory, string name) -> bool;

  auto save(Memory::Writable<n16>& memory, string name) -> bool;
  auto save(Memory::Writable<n8 >& memory, string name) -> bool;

  maybe<Cartridge&> cartridge;
};

}
