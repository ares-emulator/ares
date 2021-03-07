CartridgeSlot cartridgeSlot{"Cartridge Slot", "Cartridge"};
CartridgeSlot expansionSlot{"Expansion Slot", "Expansion"};

CartridgeSlot::CartridgeSlot(string name, string type) : name(name), type(type) {
}

auto CartridgeSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Mega Drive");
  port->setType(type);
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { return cartridge.connect(); });
  port->setDisconnect([&] { return cartridge.disconnect(); });
}

auto CartridgeSlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}
