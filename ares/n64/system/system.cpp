#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Scheduler scheduler;
System system;
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) rdp.refresh();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};

  node = Node::System::create(interface->name());
  root = node;

  scheduler.reset();
  rdram.load(node);
  cpu.load(node);
  rdp.load(node);
  rsp.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  controllerPort3.load(node);
  controllerPort4.load(node);
}

auto System::unload() -> void {
  if(!node) return;
  save();
  rdram.unload();
  cpu.unload();
  rdp.unload();
  rsp.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  controllerPort3.unload();
  controllerPort4.unload();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  cartridge.power();
  rdram.power();
  cpu.power();
  rdp.power();
  rsp.power();
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
