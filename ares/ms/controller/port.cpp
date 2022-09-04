ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Master System");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Gamepad", "Light Phaser", "Paddle", "Sports Pad", "MD Control Pad", "MD Fighting Pad"});
}

auto ControllerPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Gamepad") device = new Gamepad(port);
  if(name == "Light Phaser") device = new LightPhaser(port);
  if(name == "Paddle") device = new Paddle(port);
  if(name == "Sports Pad") device = new SportsPad(port);
  if(name == "MD Control Pad") device = new MdControlPad(port);
  if(name == "MD Fighting Pad") device = new MdFightingPad(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::power() -> void {
  trDirection = 1;
  thDirection = 1;
  trLevel = 1;
  thLevel = 1;
}

auto ControllerPort::serialize(serializer& s) -> void {
  s(trDirection);
  s(thDirection);
  s(trLevel);
  s(thLevel);
}
