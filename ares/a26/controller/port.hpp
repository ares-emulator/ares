struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  //port.cpp
  ControllerPort(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;
  static auto create(Node::Port, string name) -> std::unique_ptr<Controller>;

  auto poll() -> void { if(device) device->poll(); }
  auto read() -> n8 { if(device) return device->read(); return 0xff; }
  auto write(n8 data) -> void {
    output = data;
    if(device) device->write(data);
  }
  auto controlWrite(n4 data) -> void {
    if(device) device->controlWrite(data);
  }
  auto readAnalog(n1 index) -> Controller::AnalogConnection {
    if(device) return device->readAnalog(index);
    return Controller::AnalogConnection::disconnected();
  }

  auto serialize(serializer&) -> void;

  const string name;
  n4 output = 0x0f;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
