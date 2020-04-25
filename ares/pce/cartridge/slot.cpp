CartridgeSlot cartridgeSlot{"Cartridge Slot"};

CartridgeSlot::CartridgeSlot(string name) : name(name) {
}

auto CartridgeSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("PC Engine");
  port->setType("Cartridge");
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { return cartridge.connect(); });
  port->setDisconnect([&] { return cartridge.disconnect(); });
}

auto CartridgeSlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}

auto CartridgeSlot::power() -> void {
  if(port->connected()) cartridge.power();
}

auto CartridgeSlot::serialize(serializer& s) -> void {
  if(port->connected()) cartridge.serialize(s);
}
