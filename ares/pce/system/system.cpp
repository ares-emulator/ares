#include <pce/pce.hpp>

namespace ares::PCEngine {

Scheduler scheduler;
System system;
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) vdp.refresh();
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "PC Engine" ) information.model = Model::PCEngine;
  if(interface->name() == "SuperGrafx") information.model = Model::SuperGrafx;

  node = Node::append<Node::System>(nullptr, from, interface->name());
  root = node;

  scheduler.reset();
  cpu.load(node, from);
  vdp.load(node, from);
  psg.load(node, from);
  cartridgeSlot.load(node, from);
  controllerPort.load(node, from);
  if(PCD::Present()) pcd.load(node, from);
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  if(PCD::Present()) pcd.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  vdp.unload();
  psg.unload();
  cartridgeSlot.unload();
  controllerPort.unload();
  if(PCD::Present()) pcd.unload();
  node = {};
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  cartridge.power();
  cpu.power();
  vdp.power();
  psg.power();
  if(PCD::Present()) pcd.power();
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
