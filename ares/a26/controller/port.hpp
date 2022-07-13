struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto read() -> n8 { if(device) return device->read(); return 0xff; }
  auto write(n8 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
