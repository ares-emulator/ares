#include <ps1/ps1.hpp>

namespace ares::PlayStation {

System system;
#include "serialization.cpp"

auto System::run() -> void {
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};

  node = Node::System::create(interface->name());
  root = node;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
