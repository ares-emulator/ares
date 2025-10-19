ControllerPort controllerPort1{"Controller Port 1"};
ControllerPort controllerPort2{"Controller Port 2"};
ControllerPort controllerPort3{"Controller Port 3"};
ControllerPort controllerPort4{"Controller Port 4"};

ControllerPort::ControllerPort(string name) : name(name) {
}

auto ControllerPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Nintendo 64");
  port->setType("Controller");

  if(Model::Aleck64()) {
    port->setHotSwappable(false);
    port->setSupported({"Aleck64"});
  } else {
    port->setHotSwappable(true);
    port->setSupported({"Gamepad", "Mouse"});
  }

  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
}

auto ControllerPort::unload() -> void {
  device = {};
  port = {};
}

auto ControllerPort::save() -> void {
  if(device) device->save();
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Gamepad") device = std::make_unique<Gamepad>(port);
  if(name == "Mouse"  ) device = std::make_unique<Mouse>(port);
  if(name == "Aleck64") device = std::make_unique<Aleck64Controls>(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::serialize(serializer& s) -> void {
  if(device) s(*device);
}
