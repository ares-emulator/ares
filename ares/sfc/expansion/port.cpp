ExpansionPort expansionPort{"Expansion Port"};

ExpansionPort::ExpansionPort(string_view name) : name(name) {
}

auto ExpansionPort::load(Node::Object parent, Node::Object from) -> void {
  port = Node::append<Node::Port>(parent, from, name);
  port->setFamily("Super Famicom");
  port->setType("Expansion");
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
  port->scan(from);
}

auto ExpansionPort::unload() -> void {
  disconnect();
  port = {};
}

auto ExpansionPort::connect(Node::Peripheral node) -> void {
  disconnect();
  if(node) {
    if(node->name() == "Satellaview") device = new Satellaview(port, node);
    if(node->name() == "21fx"       ) device = new S21FX(port, node);
  }
}

auto ExpansionPort::disconnect() -> void {
  device = {};
}

auto ExpansionPort::serialize(serializer& s) -> void {
}
