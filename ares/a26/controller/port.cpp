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
  port->setSupported({
    "Gamepad", "Paddles", "Driving", "Keyboard",
    "Booster Grip", "Sega Genesis", "Joy 2B+",
    "CX-22 Trak-Ball", "CX-80 Trak-Ball", "Atari Mouse", "Amiga Mouse", "XG-1 Light Gun",
  });
  output = 0x0f;
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
  output = 0x0f;
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  device = create(port, name);
  if(device) {
    device->write(output);
    return device->node;
  }
  return {};
}

auto ControllerPort::create(Node::Port port, string name) -> std::unique_ptr<Controller> {
  if(name == "Gamepad")             return std::make_unique<Gamepad>    (port);
  if(name == "Paddles")             return std::make_unique<Paddles>    (port);
  if(name == "Driving")             return std::make_unique<Driving>    (port);
  if(name == "Keyboard")            return std::make_unique<Keyboard>   (port);
  if(name == "Booster Grip")        return std::make_unique<BoosterGrip>(port);
  if(name == "Sega Genesis")        return std::make_unique<SegaGenesis>(port);
  if(name == "Joy 2B+")             return std::make_unique<Joy2BPlus>  (port);
  if(name == "CX-22 Trak-Ball")     return std::make_unique<TrakBall>   (port, name);
  if(name == "CX-80 Trak-Ball")     return std::make_unique<TrakBall>   (port, name);
  if(name == "Atari Mouse")         return std::make_unique<AtariMouse> (port);
  if(name == "Amiga Mouse")         return std::make_unique<AmigaMouse> (port);
  if(name == "XG-1 Light Gun")      return std::make_unique<XG1LightGun>(port);
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
  if(device) device->serialize(s);
}
