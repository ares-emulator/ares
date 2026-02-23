struct Card {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Writable<n8> ram;

  struct Debugger {
    Card& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;
  } debugger{*this};

  //card.cpp
  Card(Node::Port);
  ~Card();
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

#include "slot.hpp"
