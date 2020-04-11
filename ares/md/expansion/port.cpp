ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Mega Drive");
  port->setType("Cartridge");  //not technically a cartridge, but pin-compatible with one
  port->setAllocate([&](auto name) { return expansion.allocate(port); });
  port->setConnect([&] { return expansion.connect(); });
  port->setDisconnect([&] { return expansion.disconnect(); });
}

auto ExpansionPort::unload() -> void {
  expansion.disconnect();
  port = {};
}
