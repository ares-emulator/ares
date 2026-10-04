CartridgeSlot cartridgeSlot;

auto CartridgeSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Cartridge Slot");
  port->setFamily(system.name());
  port->setType("Cartridge");
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { return cartridge.connect(); });
  port->setDisconnect([&] { return cartridge.disconnect(); });
}

auto CartridgeSlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}
