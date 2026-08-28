ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily(system.name());
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({"Gamepad", "Paddles", "Driving", "Keyboard"});
  output = 0x0f;
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
  output = 0x0f;
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  device = {};
  if(name == "Gamepad") device = std::make_unique<Gamepad>(port);
  if(name == "Paddles") device = std::make_unique<Paddles>(port);
  if(name == "Driving") device = std::make_unique<Driving>(port);
  if(name == "Keyboard") device = std::make_unique<Keyboard>(port);
  if(device) {
    device->write(output);
    return device->node;
  }
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
  if(device) device->serialize(s);
}
