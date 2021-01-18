struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto power() -> void;

  auto read() -> n8 { if(device) return device->read(); return 0xff; }
  auto write(n8 data) -> void { if(device) return device->write(data); }

  auto serialize(serializer&) -> void;

  const string name;

  struct IO {
    n1 trInputEnable = 1;
    n1 thInputEnable = 1;
    n1 trOutputLevel;
    n1 thOutputLevel;
    n1 thPreviousLevel;
  } io;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
