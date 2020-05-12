#include <n64/n64.hpp>

namespace ares::Nintendo64 {

System system;
#include "serialization.cpp"

auto System::run() -> void {
  while(!vi.refreshed) cpu.main();
  vi.refreshed = false;
  vi.refresh();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};

  node = Node::System::create(interface->name());
  root = node;

  vi.load(node);
  ai.load(node);
  pi.load(node);
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
  vi.unload();
  ai.unload();
  pi.unload();
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
  mi.power();
  vi.power();
  ai.power();
  pi.power();
  ri.power();
  si.power();
  rdram.power();
  cpu.power();
  rdp.power();
  rsp.power();

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
