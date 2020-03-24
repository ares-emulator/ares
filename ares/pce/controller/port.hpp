struct ControllerPort {
  Node::Port port;

  ControllerPort(string_view name);
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto read() -> uint4 { if(device) return device->read(); return 0b1111; }
  auto write(uint2 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;
  unique_pointer<Controller> device;
  friend class Controller;
};

extern ControllerPort controllerPort;
