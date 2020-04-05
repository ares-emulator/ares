struct Board {
  struct BlockMemory {
    BlockMemory(uint8_t* data, uint size) : data(data), size(size) {}
    BlockMemory() : data(nullptr), size(0u), writable(false) {}
    ~BlockMemory() { if(data) delete[] data; }

    auto read(uint address) const -> uint8;
    auto write(uint address, uint8 data) -> void;

    string name;
    uint8_t* data = nullptr;
    uint size = 0;
    bool writable = false;
  };

  virtual ~Board() = default;

  static auto mirror(uint addr, uint size) -> uint;

  Board(Markup::Node document = {});
  virtual auto load() -> void;
  virtual auto save() -> void;

  virtual auto main() -> void;
  virtual auto tick() -> void;

  virtual auto readPRG(uint address) -> uint8 { return 0x00; }
  virtual auto writePRG(uint address, uint8 data) -> void {}

  virtual auto readCHR(uint address) -> uint8;
  virtual auto writeCHR(uint address, uint8 data) -> void;

  virtual auto scanline(uint y) -> void {}

  virtual auto power() -> void;

  virtual auto serialize(serializer&) -> void;

  static auto load(string manifest) -> Board*;

  struct Information {
    string type;
  } information;

  BlockMemory prgrom;
  BlockMemory prgram;
  BlockMemory chrrom;
  BlockMemory chrram;

protected:
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}

  auto load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool;
  auto load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;
  auto save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool;
};
