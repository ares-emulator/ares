struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  ControllerPort(string name);
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto readControl() -> n8 { return control; }
  auto writeControl(n8 data) -> void { control = data; }

  auto readData() -> n8 { if(device) return device->readData(); return 0xff; }
  auto writeData(n8 data) -> void { if(device) return device->writeData(data); }

  auto power() -> void;
  auto serialize(serializer&) -> void;

protected:
  const string name;
  n8 control;
  friend class Controller;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
extern ControllerPort extensionPort;
