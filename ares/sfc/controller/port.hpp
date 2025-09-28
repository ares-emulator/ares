struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto iobit() -> bool { if(device) return device->iobit(); return 0; }
  auto iobit(n1 data) -> void { if(device) return device->iobit(data); }
  auto data() -> n2 { if(device) return device->data(); return 0; }
  auto latch(n1 data) -> void { if(device) return device->latch(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
