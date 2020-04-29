ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};
ControllerPort extensionPort{"Extension Port"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Mega Drive");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Control Pad", "Fighting Pad"});
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Control Pad" ) device = new ControlPad(port);
  if(name == "Fighting Pad") device = new FightingPad(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::power() -> void {
  control = 0x00;
}

auto ControllerPort::serialize(serializer& s) -> void {
  s.integer(control);
}
