struct ExpansionPort {
  Node::Port port;
  unique_pointer<Expansion> device;

  ExpansionPort(string name);
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto power() -> void;

  auto serialize(serializer&) -> void;

  const string name;
};

extern ExpansionPort expansionPort;
