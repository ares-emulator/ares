struct ExpansionPort {
  Node::Port port;
  Expansion expansion;

  //port.cpp
  ExpansionPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;

  const string name;
};

extern ExpansionPort expansionPort;
