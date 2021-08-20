namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load() -> void {}
  virtual auto save() -> void {}
  virtual auto unload() -> void {}
  virtual auto read(n16 address) -> maybe<n8> { return nothing; }
  virtual auto write(n16 address, n8 data) -> bool { return false; }
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n8>&, string name) -> bool;
  auto load(Memory::Writable<n8>&, string name) -> bool;
  auto save(Memory::Writable<n8>&, string name) -> bool;

  Cartridge& cartridge;
};

}
