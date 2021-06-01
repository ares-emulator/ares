//Famicom Disk System

#include "drive.hpp"
#include "timer.hpp"
#include "audio.hpp"

struct FDS {
  Node::Port port;
  Node::Peripheral node;
  Node::Setting::String state;
  VFS::Pak pak;
  n1 present;

  auto title() const -> string { return information.title; }

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
  FDSDrive drive;
  FDSTimer timer;
  FDSAudio audio;

  struct Information {
    string title;
  } information;
};

extern FDS fds;
