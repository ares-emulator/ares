namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load() -> void {}
  virtual auto save() -> void {}
  virtual auto unload() -> void {}
  virtual auto readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 { return data; }
  virtual auto writeP(n1 upper, n1 lower, n24 address, n16 data) -> void {}
  virtual auto readM(n32 address) -> n8 { return 0xff; }
  virtual auto readC(n32 address) -> n8 { return 0xff; }
  virtual auto readS(n32 address) -> n8 { return 0xff; }
  virtual auto readVA(n32 address) -> n8 { return 0xff; }
  virtual auto readVB(n32 address) -> n8 { return 0xff; }
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}

  auto load(Memory::Readable<n8>&, string name) -> bool;
  auto load(Memory::Readable<n16>&, string name) -> bool;
  auto load(Memory::Writable<n8>&, string name) -> bool;
  auto save(Memory::Writable<n8>&, string name) -> bool;

  Cartridge& cartridge;
};

}
