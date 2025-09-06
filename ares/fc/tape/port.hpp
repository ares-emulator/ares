struct TapePort {
  Node::Port port;
  unique_pointer<Tape> device;

  TapePort(string name);

  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto read() -> n1 { if(device) return device->read(); return 0; }
  auto write(n3 data) -> void { if(device) return device->write(data); }
  auto serialize(serializer &s) -> void { if (device) device->serialize(s); }

  const string name;
};