struct ControllerPort {
  Node::Port port;

  ControllerPort(string_view name);
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;

  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto data() -> uint3 { if(device) return device->data(); return 0b000; }
  auto latch(uint1 data) -> void { if(device) return device->latch(data); }

  auto serialize(serializer&) -> void;

private:
  const string name;
  unique_pointer<Controller> device;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
