#include <fc/fc.hpp>

namespace ares::Famicom {

FDS fds;
#include "drive.cpp"
#include "timer.cpp"
#include "audio.cpp"
#include "serialization.cpp"

auto FDS::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Disk Slot");
  port->setFamily("Famicom Disk");
  port->setType("Floppy Disk");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(port); });
  port->setConnect([&] { return connect(); });
  port->setDisconnect([&] { return disconnect(); });

  audio.load(parent);
  power();
}

auto FDS::unload() -> void {
  audio.unload();
  disconnect();
  port = {};
  disk1.sideA.reset();
  disk1.sideB.reset();
  disk2.sideA.reset();
  disk2.sideB.reset();
  inserted.reset();
  inserting.reset();
  changed = 0;
}

auto FDS::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Famicom Disk");
}

auto FDS::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  state = node->append<Node::String>("State", "Ejected", [&](auto value) {
    change(value);
  });
  vector<string> states = {"Ejected"};

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();

  if(auto fp = platform->open(node, "disk1.sideA", File::Read, File::Required)) {
    disk1.sideA.allocate(fp->size());
    disk1.sideA.load(fp);
    states.append("Disk 1: Side A");
  }

  if(auto fp = platform->open(node, "disk1.sideB", File::Read)) {
    disk1.sideB.allocate(fp->size());
    disk1.sideB.load(fp);
    states.append("Disk 1: Side B");
  }

  if(auto fp = platform->open(node, "disk2.sideA", File::Read)) {
    disk2.sideA.allocate(fp->size());
    disk2.sideA.load(fp);
    states.append("Disk 2: Side A");
  }

  if(auto fp = platform->open(node, "disk2.sideB", File::Read)) {
    disk2.sideB.allocate(fp->size());
    disk2.sideB.load(fp);
    states.append("Disk 2: Side B");
  }

  state->setAllowedValues(states);
  state->setDynamic(true);
  change(state->value());
}

auto FDS::disconnect() -> void {
  if(!node) return;

  if(disk1.sideA)
  if(auto fp = platform->open(node, "disk1.sideA", File::Write)) {
    disk1.sideA.save(fp);
  }

  if(disk1.sideB)
  if(auto fp = platform->open(node, "disk1.sideB", File::Write)) {
    disk1.sideB.save(fp);
  }

  if(disk2.sideA)
  if(auto fp = platform->open(node, "disk2.sideA", File::Write)) {
    disk2.sideA.save(fp);
  }

  if(disk2.sideB)
  if(auto fp = platform->open(node, "disk2.sideB", File::Write)) {
    disk2.sideB.save(fp);
  }

  node = {};
}

auto FDS::change(string value) -> void {
  //this setting can be changed even when the system is not powered on
  if(state) state->setLatch();

  inserting.reset();
  if(value == "Disk 1: Side A") inserting = disk1.sideA;
  if(value == "Disk 1: Side B") inserting = disk1.sideB;
  if(value == "Disk 2: Side A") inserting = disk2.sideA;
  if(value == "Disk 2: Side B") inserting = disk2.sideB;
  changed = 1;
}

auto FDS::change() -> void {
  if(changed) {
    changed = 0;
    inserted = inserting;
    inserting.reset();
    drive.changing = 1;
  }
}

auto FDS::poll() -> void {
  cpu.irqLine((timer.irq && timer.pending) || (drive.irq && drive.pending));
}

auto FDS::main() -> void {
  change();
  drive.clock();
  audio.clock();
  timer.clock();
}

auto FDS::power() -> void {
  change();
  drive = {};  //clears drive.changing; no need to wait after power-on
  timer = {};
  audio.power();
}

auto FDS::read(uint16 address, uint8 data) -> uint8 {
  data = drive.read(address, data);
  data = timer.read(address, data);
  data = audio.read(address, data);
  return data;
}

auto FDS::write(uint16 address, uint8 data) -> void {
  drive.write(address, data);
  timer.write(address, data);
  audio.write(address, data);
}

}
