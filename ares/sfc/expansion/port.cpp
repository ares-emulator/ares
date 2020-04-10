ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily("Super Famicom");
  port->setType("Expansion");
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
}

auto ExpansionPort::unload() -> void {
  disconnect();
  port = {};
}

auto ExpansionPort::connected() const -> bool {
  return (bool)device;
}

auto ExpansionPort::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    if(node->name() == "Satellaview") device = new Satellaview(port);
    if(node->name() == "21fx"       ) device = new S21FX(port);
  }
}

auto ExpansionPort::disconnect() -> void {
  device = {};
}

auto ExpansionPort::serialize(serializer& s) -> void {
}
