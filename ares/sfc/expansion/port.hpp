struct ExpansionPort {
  Node::Port port;

  ExpansionPort(string_view name);
  auto load(Node::Object parent, Node::Object from) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto serialize(serializer&) -> void;

  const string name;
  unique_pointer<Expansion> device;
};

extern ExpansionPort expansionPort;
