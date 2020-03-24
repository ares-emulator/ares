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

  //mapper.cpp
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

  struct Mapper {
    //$fffc
    uint2 shift;
    uint1 ramPage2;
    uint1 ramEnablePage2;
    uint1 ramEnablePage3;
    uint1 romWriteEnable;

    //$fffd
    uint8 romPage0;

    //$fffe
    uint8 romPage1;

    //$ffff
    uint8 romPage2;
  } mapper;
};

extern Cartridge cartridge;
