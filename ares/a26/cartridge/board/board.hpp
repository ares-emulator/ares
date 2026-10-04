namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load() -> void {}
  virtual auto save() -> void {}
  virtual auto unload() -> void {}
  virtual auto read(n16 address, n8 data) -> n8 { return data; }
  virtual auto write(n16 address, n8 data) -> n8 { return data; }
  virtual auto power(bool reset) -> void {}
  virtual auto serialize(serializer&) -> void {}

  virtual auto armInvocation() const -> Harmony::Invocation { return {}; }
  virtual auto readARM(u32 mode, n32 address, n32& data) -> Harmony::Access {
    return Harmony::Access::Unmapped;
  }
  virtual auto writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access {
    return Harmony::Access::Unmapped;
  }
  virtual auto trapARM(u32 address, n32& value, n32 argument) -> bool { return false; }
  virtual auto stepARM(u32 clocks) -> void {}

  auto load(Memory::Readable<n8>&, string name) -> bool;
  auto load(Memory::Writable<n8>&, string name) -> bool;
  auto save(Memory::Writable<n8>&, string name) -> bool;

  auto readARMMemory(Memory::Readable<n8>& rom, Memory::Writable<n8>& ram,
    u32 mode, n32 address, u32 romBase, u32 ramLimit, n32& data) -> Harmony::Access;
  auto writeARMMemory(Memory::Writable<n8>& ram, u32 mode, n32 address,
    u32 ramLimit, n32 data) -> Harmony::Access;

  Cartridge& cartridge;
};

}
