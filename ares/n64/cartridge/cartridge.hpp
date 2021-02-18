struct Cartridge {
  Node::Peripheral node;
  Memory::Readable rom;
  Memory::Writable ram;
  Memory::Writable eeprom;
  struct Flash : Memory::Writable {
    //flash.cpp
    auto readHalf(u32 address) -> u16;
    auto readWord(u32 address) -> u32;
    auto writeHalf(u32 address, u16 value) -> void;
    auto writeWord(u32 address, u32 value) -> void;

    enum class Mode : uint { Idle, Erase, Write, Read, Status };
    Mode mode = Mode::Idle;
    u64  status = 0;
    u32  source = 0;
    u32  offset = 0;
  } flash;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }
  auto cic() const -> string { return information.cic; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string manifest;
    string name;
    string region;
    string cic;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
