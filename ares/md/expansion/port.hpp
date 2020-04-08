struct ExpansionPort {
  ExpansionPort(string_view name);
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;
  auto connected() const -> bool { return (bool)expansion.node; }

  const string name;
  Node::Port port;
  Expansion expansion;
};

extern ExpansionPort expansionPort;
