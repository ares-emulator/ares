struct ExpansionPort {
  Node::Port port;

  ExpansionPort(string name);
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto connected() const -> bool;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto serialize(serializer&) -> void;

  const string name;
  unique_pointer<Expansion> device;
};

extern ExpansionPort expansionPort;
