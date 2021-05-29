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
  virtual auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 { return data; }
  virtual auto write(n1 upper, n1 lower, n22 address, n16 data) -> void {}
  virtual auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 { return data; }
  virtual auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {}
  virtual auto vblank(bool line) -> void {}
  virtual auto hblank(bool line) -> void {}
  virtual auto power(bool reset) -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n16>& rom, string name) -> bool;
  auto load(Memory::Writable<n16>& wram, Memory::Writable<n8>& uram, Memory::Writable<n8>& lram, string name) -> bool;
  auto load(M24C& m24c, string name) -> bool;

  auto save(Memory::Writable<n16>& wram, Memory::Writable<n8>& uram, Memory::Writable<n8>& lram, string name) -> bool;
  auto save(M24C& m24c, string name) -> bool;

  maybe<Cartridge&> cartridge;
};

}
