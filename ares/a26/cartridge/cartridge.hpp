struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }
  auto region() const -> string { return information.region; }
  auto sha256() const -> string { return information.sha256; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power(bool reset) -> void;

  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> n8;

  auto armInvocation() const -> Harmony::Invocation;
  auto readARM(u32 mode, n32 address, n32& data) -> Harmony::Access;
  auto writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access;
  auto trapARM(u32 address, n32& value, n32 argument) -> bool;
  auto stepARM(u32 clocks) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  std::unique_ptr<Board::Interface> board;

//private:
  struct Information {
    string title;
    string region;
    string board;
    string sha256;
    bool phosphor;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
