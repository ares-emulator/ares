struct PeripheralPort {
  Node::Port port;
  unique_pointer<PeripheralDevice> device;

  //port.cpp
  PeripheralPort(string name, string type);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto reset() -> void;
  auto acknowledge() -> bool;
  auto bus(u8 data) -> u8;

  auto serialize(serializer&) -> void;

  const string name;
  const string type;
};

extern PeripheralPort controllerPort1;
extern PeripheralPort memoryCardPort1;
extern PeripheralPort controllerPort2;
extern PeripheralPort memoryCardPort2;
