ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Famicom");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({"Gamepad", "Zapper"});
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Gamepad") device = std::make_unique<Gamepad>(port);
  if(name == "Zapper") device = std::make_unique<Zapper>(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
  if(device) device->serialize(s);
}
