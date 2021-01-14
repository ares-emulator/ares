//Famicom Disk System

#include "drive.hpp"
#include "timer.hpp"
#include "audio.hpp"

struct FDS {
  Node::Port port;
  Node::Peripheral node;
  Node::Setting::String state;
  n1 present;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  struct Disk {
    Memory::Writable<n8> sideA;
    Memory::Writable<n8> sideB;
  };
  Disk disk1;
  Disk disk2;
  maybe<Memory::Writable<n8>&> inserting;
  maybe<Memory::Writable<n8>&> inserted;
  n1 changed;

  //fds.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto change(string value) -> void;
  auto change() -> void;

  auto poll() -> void;
  auto main() -> void;
  auto power() -> void;

  auto read(n16 address, n8 data) -> n8;
  auto write(n16 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string manifest;
    string name;
  } information;

  FDSDrive drive;
  FDSTimer timer;
  FDSAudio audio;
};

extern FDS fds;
