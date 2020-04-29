ControllerPort controllerPort{"Controller Port"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("PC Engine");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Gamepad", "Avenue Pad 6"});
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Gamepad") device = new Gamepad(port);
  if(name == "Avenue Pad 6") device = new AvenuePad(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
}
