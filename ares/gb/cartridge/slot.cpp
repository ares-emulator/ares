CartridgeSlot cartridgeSlot{"Cartridge Slot"};

CartridgeSlot::CartridgeSlot(string name) : name(name) {
}

auto CartridgeSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily(!Model::GameBoyColor() ? "Game Boy" : "Game Boy Color");
  port->setType("Cartridge");
  port->setAllocate([&] { return Node::Peripheral::create(port->family()); });
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
}

auto CartridgeSlot::unload() -> void {
  disconnect();
  port = {};
}

auto CartridgeSlot::connected() const -> bool {
  return (bool)cartridge.node;
}

auto CartridgeSlot::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    cartridge.connect(port, node);
  }
}

auto CartridgeSlot::disconnect() -> void {
  if(connected()) {
    cartridge.disconnect();
  }
}
