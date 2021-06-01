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
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  state = node->append<Node::Setting::String>("State", "Ejected", [&](auto value) {
    change(value);
  });
  vector<string> states = {"Ejected"};

  if(auto fp = pak->read("disk1.sideA")) {
    disk1.sideA.allocate(fp->size());
    disk1.sideA.load(fp);
    states.append("Disk 1: Side A");
  }

  if(auto fp = pak->read("disk1.sideB")) {
    disk1.sideB.allocate(fp->size());
    disk1.sideB.load(fp);
    states.append("Disk 1: Side B");
  }

  if(auto fp = pak->read("disk2.sideA")) {
    disk2.sideA.allocate(fp->size());
    disk2.sideA.load(fp);
    states.append("Disk 2: Side A");
  }

  if(auto fp = pak->read("disk2.sideB")) {
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
  if(auto fp = pak->write("disk1.sideA")) {
    disk1.sideA.save(fp);
  }

  if(disk1.sideB)
  if(auto fp = pak->write("disk1.sideB")) {
    disk1.sideB.save(fp);
  }

  if(disk2.sideA)
  if(auto fp = pak->write("disk2.sideA")) {
    disk2.sideA.save(fp);
  }

  if(disk2.sideB)
  if(auto fp = pak->write("disk2.sideB")) {
    disk2.sideB.save(fp);
  }

  pak.reset();
  node.reset();
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
  audio.power();
}

auto FDS::read(n16 address, n8 data) -> n8 {
  data = drive.read(address, data);
  data = timer.read(address, data);
  data = audio.read(address, data);
  return data;
}

auto FDS::write(n16 address, n8 data) -> void {
  drive.write(address, data);
  timer.write(address, data);
  audio.write(address, data);
}

}
