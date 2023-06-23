struct Tape : Thread {
  Node::Peripheral node;
  Node::Audio::Stream stream;

  auto title() const -> string { return information.title; }

  //tape.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto save() -> void;

private:
  struct Information {
    string title;
  } information;

  VFS::Pak pak;
  u64 position;
  u64 length;
  u64 range;
  u64 frequency;
  Memory::Writable<u64> data;

};

#include "tray.hpp"
#include "deck.hpp"
