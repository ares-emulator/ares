BSMemorySlot bsmemorySlot{"BS Memory Slot"};

BSMemorySlot::BSMemorySlot(string name) : name(name) {
}

auto BSMemorySlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("BS Memory");
  port->setType("Cartridge");
  port->setAllocate([&] { return Node::Peripheral::create("BS Memory"); });
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
}

auto BSMemorySlot::unload() -> void {
  disconnect();
  port = {};
}

auto BSMemorySlot::connected() const -> bool {
  return (bool)cartridge.node;
}

auto BSMemorySlot::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    cartridge.connect(port, node);
  }
}

auto BSMemorySlot::disconnect() -> void {
  if(connected()) {
    cartridge.disconnect();
  }
}
