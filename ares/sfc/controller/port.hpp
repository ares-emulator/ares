struct ControllerPort {
  Node::Port port;

  ControllerPort(string_view name);
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto iobit() -> bool { if(device) return device->iobit(); return 0; }
  auto iobit(bool data) -> void { if(device) return device->iobit(data); }
  auto data() -> uint2 { if(device) return device->data(); return 0; }
  auto latch(bool data) -> void { if(device) return device->latch(data); }

  auto serialize(serializer&) -> void;

  const string name;
  unique_pointer<Controller> device;
  friend class Controller;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
