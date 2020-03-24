struct Cartridge {
  Node::Port port;
  Node::Peripheral node;

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }
  inline auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(uint16 address) -> maybe<uint8>;
  auto write(uint16 address, uint8 data) -> bool;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Information {
    string manifest;
    string name;
    string region;
  } information;

  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;
};

extern Cartridge cartridge;
