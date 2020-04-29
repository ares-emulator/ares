ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Super Famicom");
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({
    "Gamepad",
    "Justifier",
    "Justifiers",
    "Mouse",
    "NTT Data Keypad",
    "Super Multitap",
    "Super Scope",
    "Twin Tap"
  });
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Gamepad"        ) device = new Gamepad(port);
  if(name == "Justifier"      ) device = new Justifier(port);
  if(name == "Justifiers"     ) device = new Justifiers(port);
  if(name == "Mouse"          ) device = new Mouse(port);
  if(name == "NTT Data Keypad") device = new NTTDataKeypad(port);
  if(name == "Super Multitap" ) device = new SuperMultitap(port);
  if(name == "Super Scope"    ) device = new SuperScope(port);
  if(name == "Twin Tap"       ) device = new TwinTap(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
}
