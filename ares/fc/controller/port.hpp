struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto data() -> uint3 { if(device) return device->data(); return 0b000; }
  auto latch(uint1 data) -> void { if(device) return device->latch(data); }

  auto serialize(serializer&) -> void;

  const string name;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
