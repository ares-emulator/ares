ControllerPort controllerPort{"Controller Port"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("PC Engine");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
}

auto ControllerPort::unload() -> void {
  disconnect();
  port = {};
}

auto ControllerPort::connected() const -> bool {
  return (bool)device;
}

auto ControllerPort::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    if(node->name() == "Gamepad") device = new Gamepad(port, node);
    if(node->name() == "Avenue Pad 6") device = new AvenuePad(port, node);
  }
}

auto ControllerPort::disconnect() -> void {
  device = {};
}

auto ControllerPort::serialize(serializer& s) -> void {
}
