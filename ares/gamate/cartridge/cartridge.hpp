//every Gamate cartridge carries a protection chip that hides the ROM until the
//console has clocked the string "BIT CORPORATION" into it one bit at a time and
//clocked a reply back out. only after that does the cartridge answer with data.
//
//the three board types differ solely in how they decode the 32KB window, so
//they are an enum here rather than a class hierarchy

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return information.title; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(n16 offset) -> n8;
  auto write(n16 offset, n8 data) -> void;

  //these three ports live on the main board but appear to speak to the
  //cartridge; what they actually gate is not understood
  auto cardAvailableSet() -> n8;
  auto cardAvailableCheck() -> n8;
  auto cardReset() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum class Board : u32 { Plain, Banked, FourInOne };

private:
  auto readROM(n16 offset) -> n8;
  auto writeROM(n16 offset, n8 data) -> void;

  struct Information {
    string title;
    Board  board = Board::Plain;
  } information;

  //the serial handshake that unlocks the cartridge
  struct Protection {
    auto power() -> void;
    auto read() -> n1;
    auto write(n1 state) -> void;

    n1 unlocked;   //the ROM is readable
    n1 failed;     //a wrong byte arrived; the chip stays shut until power off
    n1 replying;   //the key matched and the answer is being shifted out
    n4 position;   //bit position within the byte being shifted
    n8 byte;       //the byte being assembled, or the reply being shifted out
    n8 sequence;   //how many bytes of the key have arrived
  } protection;

  Memory::Readable<n8> rom;
  n8 bank;
  n8 multibank;
  n1 cardAvailable;
};

#include "slot.hpp"
extern Cartridge& cartridge;
