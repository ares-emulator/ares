ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string_view name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent, Node::Object from) -> void {
  port = Node::append<Node::Port>(parent, from, name);
  port->setFamily("Mega Drive");
  port->setType("Cartridge");  //not technically a cartridge, but pin-compatible with one
  port->setAllocate([&] { return Node::Peripheral::create(interface->name()); });
  port->setAttach([&](auto node) { return connect(node); });
  port->setDetach([&](auto node) { return disconnect(); });
  port->scan(from);
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
