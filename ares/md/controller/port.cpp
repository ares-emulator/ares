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
  port->setDisconnect([&] { return disconnect(); });

  port->setSupported({"Control Pad", "Fighting Pad", "Mega Mouse"});
}

auto ControllerPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ControllerPort::allocate(string name) -> Node::Peripheral {
  if(name == "Control Pad" ) device = std::make_unique<ControlPad>(port);
  if(name == "Fighting Pad") device = std::make_unique<FightingPad>(port);
  if(name == "Mega Mouse") device = std::make_unique<MegaMouse>(port);
  if(device) return device->node;
  return {};
}

auto ControllerPort::update() -> void {
  auto [inputData, inputMask] = device ? device->readData() : Controller::Data{0x7f, 0x7f};
  n8 outputMask = 0x80 | control;
  n8 prevLines = dataLines;
  dataLines = inputData & inputMask | dataLines & ~inputMask;
  dataLines = dataLatch & outputMask | dataLines & ~outputMask;
  //todo: gradually pull up floating lines
  if(device && prevLines != dataLines) return device->writeData(dataLines);
}

auto ControllerPort::power(bool reset) -> void {
  if(!reset) {
    control        = 0x00;
    dataLatch      = 0x7f;
    dataLines      = 0x7f;
    serialControl  = 0x00;
    serialTxBuffer = 0xff;
    serialRxBuffer = 0x00;
  }
}

auto ControllerPort::serialize(serializer& s) -> void {
  s(control);
  s(dataLatch);
  s(dataLines);
  s(serialControl);
  s(serialTxBuffer);
  s(serialRxBuffer);
}
