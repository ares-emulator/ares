struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto poll() -> void { if(device) device->poll(); }
  auto read() -> n8 { if(device) return device->read(); return 0xff; }
  auto write(n8 data) -> void {
    output = data;
    if(device) device->write(data);
  }
  auto readAnalogA() -> Controller::AnalogConnection {
    if(device) return device->readAnalogA();
    return Controller::AnalogConnection::disconnected();
  }
  auto readAnalogB() -> Controller::AnalogConnection {
    if(device) return device->readAnalogB();
    return Controller::AnalogConnection::disconnected();
  }

  auto serialize(serializer&) -> void;

  const string name;
  n4 output = 0x0f;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
