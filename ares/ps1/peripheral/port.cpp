PeripheralPort controllerPort1{"Controller Port 1",  "Controller" };
PeripheralPort memoryCardPort1{"Memory Card Port 1", "Memory Card"};
PeripheralPort controllerPort2{"Controller Port 2",  "Controller" };
PeripheralPort memoryCardPort2{"Memory Card Port 2", "Memory Card"};

PeripheralPort::PeripheralPort(string name, string type) : name(name), type(type) {
}

auto PeripheralPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("PlayStation");
  port->setType(type);
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  if(type == "Controller") {
    port->setSupported({"Digital Gamepad"});
  }
  if(type == "Memory Card") {
    port->setSupported({"Memory Card"});
  }
}

auto PeripheralPort::unload() -> void {
  device.reset();
  port.reset();
}

auto PeripheralPort::allocate(string name) -> Node::Peripheral {
  if(name == "Digital Gamepad") device = new DigitalGamepad(port);
  if(name == "Memory Card"    ) device = new MemoryCard(port);
  if(device) return device->node;
  return {};
}

auto PeripheralPort::reset() -> void {
  if(device) return device->reset();
}

auto PeripheralPort::acknowledge() -> bool {
  if(device) return device->acknowledge();
  return 0;
}

auto PeripheralPort::bus(u8 data) -> u8 {
  if(device) return device->bus(data);
  return 0xff;
}

auto PeripheralPort::serialize(serializer& s) -> void {
}
