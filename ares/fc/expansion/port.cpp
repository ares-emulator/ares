ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Famicom");
  port->setType("Expansion");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setSupported({"Family Keyboard"});
}

auto ExpansionPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ExpansionPort::allocate(string name) -> Node::Peripheral {
  if(name == "Family Keyboard") device = new FamilyKeyboard(port);
  if(device) return device->node;
  return {};
}

auto ExpansionPort::serialize(serializer& s) -> void {
}
