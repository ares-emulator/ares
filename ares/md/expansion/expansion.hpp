//Mega CD interface:
//the only actual peripheral released for the expansion port was the Mega CD.
//this class is used to connect a peripheral folder and the BIOS to the emulator.

struct Expansion {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto regions() const -> vector<string> { return information.regions; }

  //expansion.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16;
  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void;

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
    vector<string> regions;
  } information;
};

#include "port.hpp"
extern Expansion& expansion;
