namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;

  virtual auto load() -> bool { return false; }
  virtual auto unload() -> void {}
  virtual auto power() -> void {}
  virtual auto read(n16 address, n8 data) -> n8 { return data; }
  virtual auto peek(n16 address, n8 data) const -> n8 { return data; }
  virtual auto write(n16 address, n8 data) -> bool { return false; }
  Cartridge& cartridge;
};

}
