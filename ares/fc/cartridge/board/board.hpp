namespace Board {

struct Interface {
  VFS::Pak pak;

  static auto create(string manifest) -> Interface*;

  virtual ~Interface() = default;

  virtual auto load() -> void {}
  virtual auto save() -> void {}
  virtual auto unload() -> void {}

  virtual auto main() -> void;
  virtual auto tick() -> void;

  virtual auto readPRG(n32 address, n8 data) -> n8 { return data; }
  virtual auto writePRG(n32 address, n8 data) -> void {}

  virtual auto readCHR(n32 address, n8 data) -> n8 { return data; }
  virtual auto writeCHR(n32 address, n8 data) -> void {}

  virtual auto scanline(n32 y) -> void {}

  virtual auto power() -> void {}

  virtual auto serialize(serializer&) -> void {}

protected:
  auto load(Memory::Readable<n8>& memory, string name) -> bool;
  auto load(Memory::Writable<n8>& memory, string name) -> bool;
  auto save(Memory::Writable<n8>& memory, string name) -> bool;
};

}
