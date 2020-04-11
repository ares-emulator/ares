BSMemorySlot bsmemorySlot{"BS Memory Slot"};

BSMemorySlot::BSMemorySlot(string name) : name(name) {
}

auto BSMemorySlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("BS Memory");
  port->setType("Cartridge");
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { return cartridge.connect(); });
  port->setDisconnect([&] { return cartridge.disconnect(); });
}

auto BSMemorySlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}
