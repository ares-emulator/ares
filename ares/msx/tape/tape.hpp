struct Tape : Thread {
  Node::Tape node;
  Node::Audio::Stream stream;

  auto title() const -> string { return information.title; }

  //tape.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> bool;
  auto disconnect() -> void;
  auto load() -> bool;
  auto unload() -> void;
  auto read() -> n1;
  auto write(n1 data) -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto save() -> void;
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string title;
  } information;

  VFS::Pak pak;
  u64 range;
  n1 output;
  n1 input;
  Memory::Writable<u64> data;

};

#include "tray.hpp"
#include "deck.hpp"
