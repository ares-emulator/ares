CartridgeSlot cartridgeSlot{"Cartridge Slot"};

CartridgeSlot::CartridgeSlot(string name) : name(name) {
}

auto CartridgeSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Game Boy Advance");
  port->setType("Cartridge");
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { cartridge.connect(); });
  port->setDisconnect([&] { cartridge.disconnect(); });
}

auto CartridgeSlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}
