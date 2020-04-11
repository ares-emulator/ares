SufamiTurboSlot sufamiturboSlotA{"Sufami Turbo Slot A"};
SufamiTurboSlot sufamiturboSlotB{"Sufami Turbo Slot B"};

SufamiTurboSlot::SufamiTurboSlot(string name) : name(name) {
}

auto SufamiTurboSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Sufami Turbo");
  port->setType("Cartridge");
  port->setAllocate([&](auto name) { return cartridge.allocate(port); });
  port->setConnect([&] { return cartridge.connect(); });
  port->setDisconnect([&] { return cartridge.disconnect(); });
}

auto SufamiTurboSlot::unload() -> void {
  cartridge.disconnect();
  port = {};
}
