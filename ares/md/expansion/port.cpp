ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Mega Drive");
  port->setType("Cartridge");  //not technically a cartridge, but pin-compatible with one
  port->setAllocate([&] { return Node::Peripheral::create(interface->name()); });
  port->setAttach([&](auto node) { return connect(node); });
  port->setDetach([&](auto node) { return disconnect(); });
}

auto ExpansionPort::unload() -> void {
  disconnect();
  port = {};
}

auto ExpansionPort::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    expansion.connect(port, node);
  }
}

auto ExpansionPort::disconnect() -> void {
  if(connected()) {
    expansion.disconnect();
  }
}
