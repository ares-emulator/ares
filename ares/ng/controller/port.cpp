ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Neo Geo");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Arcade Stick"});
}

auto ControllerPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Arcade Stick") device = new ArcadeStick(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::power() -> void {
}

auto ControllerPort::serialize(serializer& s) -> void {
}
