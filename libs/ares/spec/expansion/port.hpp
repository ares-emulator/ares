struct ExpansionPort {
  Node::Port port;
  std::unique_ptr<Expansion> device;

  ExpansionPort(string name);
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto connected() -> bool { return device ? true : false; }
  auto romcs() -> bool { return device ? device->romcs() : false; }
  auto mapped(n16 address, bool io) -> bool { return device ? device->mapped(address, io) : false; }
  auto read(n16 address) -> n8 { return device ? device->read(address) : (n8)0xff; }
  auto write(n16 address, n8 data) -> void { if (device) device->write(address, data); }
  auto in(n16 address) -> n8 { return device ? device->in(address) : (n8)0xff;}
  auto out(n16 address, n8 data) -> void { if (device) device->out(address, data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ExpansionPort expansionPort;
