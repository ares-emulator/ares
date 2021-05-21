CardSlot cardSlot{"Memory Card Slot"};

CardSlot::CardSlot(string name) : name(name) {
}

auto CardSlot::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Neo Geo");
  port->setType("Memory Card");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Memory Card"});
}

auto CardSlot::unload() -> void {
  device.reset();
  port.reset();
}

auto CardSlot::allocate(string name) -> Node::Peripheral {
  if(name == "Memory Card") device = new Card(port);
  if(device) return device->node;
  return {};
}

auto CardSlot::read(n24 address) -> n8 {
  if(!device || lock) return 0xff;
  return device->ram.read(address);
}

auto CardSlot::write(n24 address, n8 data) -> void {
  if(!device || lock) return;
  return device->ram.write(address, data);
}

auto CardSlot::save() -> void {
}

auto CardSlot::power(bool reset) -> void {
  lock = 3;
  select = 0;
  bank = 0;
  if(device) device->power(reset);
}
