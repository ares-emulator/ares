struct ExpansionPort {
  Node::Port port;
  unique_pointer<Expansion> device;

  ExpansionPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto read1() -> n1 { if(device) return device->read1(); return 0; }
  auto read2() -> n5 { if(device) return device->read2(); return 0; }
  auto write(n3 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ExpansionPort expansionPort;
