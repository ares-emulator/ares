SufamiTurboSlot sufamiturboSlotA{"Sufami Turbo Slot A"};
SufamiTurboSlot sufamiturboSlotB{"Sufami Turbo Slot B"};

SufamiTurboSlot::SufamiTurboSlot(string name) : name(name) {
}

auto SufamiTurboSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Sufami Turbo");
  port->setType("Cartridge");
  port->setAllocate([&] { return Node::Peripheral::create("Sufami Turbo"); });
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
}

auto SufamiTurboSlot::unload() -> void {
  disconnect();
  port = {};
}

auto SufamiTurboSlot::connected() const -> bool {
  return (bool)cartridge.node;
}

auto SufamiTurboSlot::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    cartridge.connect(port, node);
  }
}

auto SufamiTurboSlot::disconnect() -> void {
  if(connected()) {
    cartridge.disconnect();
  }
}
