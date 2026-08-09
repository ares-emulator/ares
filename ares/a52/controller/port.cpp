ControllerPort controllerPorts[4] = {
  {"Controller Port 1"},
  {"Controller Port 2"},
  {"Controller Port 3"},
  {"Controller Port 4"},
};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily(system.name());
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({"Controller"});
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Controller") device = std::make_unique<StandardController>(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::connected() const -> bool {
  return (bool)device;
}

auto ControllerPort::poll() -> void {
  if(device) device->poll();
}

auto ControllerPort::axis(u32 index, bool powered) -> s16 {
  if(device) return device->axis(index, powered);
  return 0;
}

auto ControllerPort::keypad(n4 code) -> bool {
  if(device) return device->keypad(code);
  return false;
}

auto ControllerPort::topFire() -> bool {
  if(device) return device->topFire();
  return false;
}

auto ControllerPort::bottomFire() -> bool {
  if(device) return device->bottomFire();
  return false;
}
