ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Master System");
  port->setType("Expansion");
  port->setHotSwappable(false);
  port->setAllocate([&](auto name) { return allocate(name); });
}

auto ExpansionPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ExpansionPort::allocate(string name) -> Node::Peripheral {
  if(name == "FM Sound Unit") device = new FMSoundUnit(port);
  if(device) return device->node;
  return {};
}

auto ExpansionPort::power() -> void {
}

auto ExpansionPort::serialize(serializer& s) -> void {
  if(device) return device->serialize(s);
}
