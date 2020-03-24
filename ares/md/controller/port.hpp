struct ControllerPort {
  Node::Port port;

  //port.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;

  ControllerPort(string_view name);
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto readControl() -> uint8 { return control; }
  auto writeControl(uint8 data) -> void { control = data; }

  auto readData() -> uint8 { if(device) return device->readData(); return 0xff; }
  auto writeData(uint8 data) -> void { if(device) return device->writeData(data); }

  auto power() -> void;
  auto serialize(serializer&) -> void;

  const string name;
  uint8 control;
  unique_pointer<Controller> device;
  friend class Controller;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
extern ControllerPort extensionPort;
