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
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({
    "Gamepad",
    "Rumble Gamepad",
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
  if(name == "Gamepad"        ) device = std::make_unique<Gamepad>(port);
  if(name == "Rumble Gamepad" ) device = std::make_unique<RumbleGamepad>(port);
  if(name == "Justifier"      ) device = std::make_unique<Justifier>(port);
  if(name == "Justifiers"     ) device = std::make_unique<Justifiers>(port);
  if(name == "Mouse"          ) device = std::make_unique<Mouse>(port);
  if(name == "NTT Data Keypad") device = std::make_unique<NTTDataKeypad>(port);
  if(name == "Super Multitap" ) device = std::make_unique<SuperMultitap>(port);
  if(name == "Super Scope"    ) device = std::make_unique<SuperScope>(port);
  if(name == "Twin Tap"       ) device = std::make_unique<TwinTap>(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
}
