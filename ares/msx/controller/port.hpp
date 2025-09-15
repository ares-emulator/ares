struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto read() -> n6 { if(device) return device->read(); return 0x3f; }

  // Bit  Function
  // 0    Pin 6
  // 1    Pin 7
  // 2    Pin 8
  auto write(n8 data) { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
