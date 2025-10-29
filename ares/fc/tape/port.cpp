TapePort::TapePort(string name) : name(name) {
}

auto TapePort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Famicom");
  port->setType("Tape");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({"Family BASIC Data Recorder"});
}

auto TapePort::unload() -> void {
  device.reset();
  port.reset();
}

auto TapePort::allocate(string name) -> Node::Peripheral {
  if (name == "Family BASIC Data Recorder") device = std::make_unique<FamilyBasicDataRecorder>(port);
  if (device) return device->node;
  return {};
}