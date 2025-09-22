ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("ZX Spectrum");
  port->setType("Expansion");
  port->setHotSwappable(false);
  port->setAllocate([&](auto name) { return allocate(name); });
}

auto ExpansionPort::unload() -> void {
  device = {};
  port = {};
}

auto ExpansionPort::allocate(string name) -> Node::Peripheral {
  if(name == "Kempston") device = std::make_unique<Kempston>(port);
  if(device) return device->node;
  return {};
}

auto ExpansionPort::serialize(serializer& s) -> void {
}
