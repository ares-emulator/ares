struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto read() -> uint4 { if(device) return device->read(); return 0b1111; }
  auto write(uint2 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ControllerPort controllerPort;
