namespace Board {

struct Interface {
  Interface(Markup::Node document = {});
  virtual ~Interface() = default;

  virtual auto load() -> void;
  virtual auto save() -> void;
  virtual auto unload() -> void;

  virtual auto main() -> void;
  virtual auto tick() -> void;

  virtual auto readPRG(uint address) -> uint8 { return 0x00; }
  virtual auto writePRG(uint address, uint8 data) -> void {}

  virtual auto readCHR(uint address) -> uint8 { return 0x00; }
  virtual auto writeCHR(uint address, uint8 data) -> void {}

  virtual auto scanline(uint y) -> void {}

  virtual auto power() -> void {}

  virtual auto serialize(serializer&) -> void {}

  static auto load(string manifest) -> Interface*;

  struct Information {
    string type;
  } information;

protected:
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}

  auto load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;
  auto save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;
};

}
