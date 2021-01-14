namespace Board {

struct Interface {
  static auto create(string manifest) -> Interface*;

  virtual ~Interface() = default;

  virtual auto load() -> void;
  virtual auto save() -> void;
  virtual auto unload() -> void;

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
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}

  auto load(Memory::Readable<n8>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<n8>& memory, Markup::Node node) -> bool;
  auto save(Memory::Writable<n8>& memory, Markup::Node node) -> bool;
};

}
