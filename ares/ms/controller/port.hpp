struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto power() -> void;

  auto read() -> n7 { if(device) return device->read(); return 0x7f; }
  auto write(n8 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;

  auto trInput() const -> bool { return trDirection == 1; }
  auto thInput() const -> bool { return thDirection == 1; }

  auto trOutput() const -> bool { return trDirection == 0; }
  auto thOutput() const -> bool { return thDirection == 0; }

  n1 trDirection;
  n1 thDirection;
  n1 trLevel;
  n1 thLevel;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
