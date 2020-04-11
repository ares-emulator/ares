struct Cartridge : Thread {
  Node::Peripheral node;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;
  Memory::Writable<uint8> rtc;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;
  auto second() -> void;

  //io.cpp
  auto readIO(uint cycle, uint16 address, uint8 data) -> uint8;
  auto writeIO(uint cycle, uint16 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
  } information;

  bool bootromEnable = true;

private:
  struct Mapper {
    virtual auto load(Node::Object) -> void {}
    virtual auto unload() -> void {}
    virtual auto load(Markup::Node document) -> void {}
    virtual auto save(Markup::Node document) -> void {}
    virtual auto main() -> void;
    virtual auto second() -> void {}
    virtual auto read(uint16 address) -> uint8 = 0;
    virtual auto write(uint16 address, uint8 data) -> void = 0;
    virtual auto power() -> void = 0;
    virtual auto serialize(serializer&) -> void = 0;
  };
  Mapper* mapper = nullptr;
  bool accelerometer = false;
  bool rumble = false;

  #include "mbc0/mbc0.hpp"
  #include "mbc1/mbc1.hpp"
  #include "mbc1m/mbc1m.hpp"
  #include "mbc2/mbc2.hpp"
  #include "mbc3/mbc3.hpp"
  #include "mbc5/mbc5.hpp"
  #include "mbc6/mbc6.hpp"
  #include "mbc7/mbc7.hpp"
  #include "mmm01/mmm01.hpp"
  #include "huc1/huc1.hpp"
  #include "huc3/huc3.hpp"
  #include "tama/tama.hpp"
};

#include "slot.hpp"
extern Cartridge& cartridge;
